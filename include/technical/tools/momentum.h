#ifndef MOMEMNTUM_H
#define MOMEMNTUM_H
#pragma once

// Pure TA swing + liquidity engine (spec: Price → Structure → Swings → Clusters → Zones).
// No ML, regression, probabilities, weights, or volume scores.
//
// Live constraint: every step is causal except the designed confirmation lag of `k`
// bars to the right of a pivot (the swing is emitted only once bar t+k exists).
// Centered/kernel smoothers are intentionally not used.

#include "map/Candlestick.h"
#include "technical/indicators/average_true_range.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <map>
#include <span>
#include <vector>

namespace map::technical {

    using map::market_data::Candlestick;
    using map::market_data::Timeframe;

    struct Swing {
        std::size_t index{};
        double price{};       // raw wick: high for SH, low for SL
        bool is_high{};
        Timeframe timeframe{ Timeframe::M5 };
        double prominence{};      // P  (price units)
        double prominence_atr{};  // P* = P / ATR_n  (technical ratio)
        std::chrono::year_month_day date{};
        std::chrono::milliseconds time{};
    };

    enum class LiquiditySide { Buy, Sell };

    enum class ZoneStatus {
        Active,
        Tested,
        Swept,
        Broken,
        Invalidated
    };

    struct LiquidityZone {
        LiquiditySide side{};
        double low{};
        double high{};
        Timeframe timeframe{ Timeframe::M5 };
        std::chrono::year_month_day creation_date{};
        std::chrono::milliseconds creation_time{};
        std::chrono::year_month_day last_interaction_date{};
        std::chrono::milliseconds last_interaction_time{};
        std::vector<std::size_t> source_swings;
        std::vector<std::size_t> touch_indices;
        int touch_count{ 0 };
        ZoneStatus status{ ZoneStatus::Active };
        std::size_t event_index{ static_cast<std::size_t>(-1) };
    };

    enum class SmoothingMode {
        None,
        CausalSma,  // default — no future bars
        CausalEma
    };

    struct SwingConfig {
        std::size_t k{ 2 };  // lookback = lookforward (spec window)
        SmoothingMode smoothing{ SmoothingMode::CausalSma };
        std::size_t smooth_period{ 3 };
        double prominence_atr_min{ 1.0 };  // keep swing iff P* >= this
        Timeframe timeframe{ Timeframe::M5 };
    };

    struct LiquidityConfig {
        double abs_eps{ 1e-9 };
        double atr_epsilon_k{ 0.10 };  // ε = k × ATR
        double atr_pad_k{ 0.05 };      // band padding
        std::size_t min_touches{ 2 };  // EQH/EQL: 2+ clustered swings
        std::size_t break_accept_bars{ 2 };  // consecutive closes beyond → Invalidated
        Timeframe timeframe{ Timeframe::M5 };
    };

    struct MarketStructure {
        Timeframe timeframe{ Timeframe::M5 };
        std::vector<Swing> swings;
        std::vector<LiquidityZone> zones;
    };

    namespace detail {

        inline double typical(const Candlestick& c) {
            return (c.m_high + c.m_low + c.m_close) / 3.0;
        }

        // Volatility = map::indicators::Atr (mean true range over a time window).
        inline double atr_until(const std::vector<Candlestick>& bars, std::size_t i) {
            if (bars.size() < 2 || i >= bars.size()) {
                return 0.0;
            }
            std::vector<Candlestick> prefix(bars.begin(), bars.begin() + static_cast<std::ptrdiff_t>(i) + 1);
            auto date = prefix.back().m_date;
            auto time = prefix.back().m_time;
            return map::indicators::Atr{}.get_average(prefix, date, time);
        }

        inline double atr_on_date(const std::vector<Candlestick>& bars, std::size_t i) {
            if (bars.size() < 2 || i >= bars.size()) {
                return 0.0;
            }
            std::vector<Candlestick> copy = bars;
            auto date = copy[i].m_date;
            return map::indicators::Atr{}.get_average(copy, date);
        }

        inline std::vector<double> causal_sma(const std::vector<double>& x, std::size_t period) {
            const std::size_t n = x.size();
            const std::size_t p = std::max<std::size_t>(1, period);
            std::vector<double> y(n, 0.0);
            double acc = 0.0;
            for (std::size_t i = 0; i < n; ++i) {
                acc += x[i];
                if (i >= p) {
                    acc -= x[i - p];
                }
                y[i] = acc / static_cast<double>(std::min(i + 1, p));
            }
            return y;
        }

        inline std::vector<double> causal_ema(const std::vector<double>& x, std::size_t period) {
            const std::size_t n = x.size();
            const std::size_t p = std::max<std::size_t>(1, period);
            const double a = 2.0 / (static_cast<double>(p) + 1.0);
            std::vector<double> y(n, 0.0);
            if (n == 0) {
                return y;
            }
            y[0] = x[0];
            for (std::size_t i = 1; i < n; ++i) {
                y[i] = a * x[i] + (1.0 - a) * y[i - 1];
            }
            return y;
        }

        inline std::vector<double> structure_series(std::span<const Candlestick> bars, const SwingConfig& cfg) {
            std::vector<double> px(bars.size());
            for (std::size_t i = 0; i < bars.size(); ++i) {
                px[i] = typical(bars[i]);
            }
            switch (cfg.smoothing) {
            case SmoothingMode::CausalSma:
                return causal_sma(px, cfg.smooth_period);
            case SmoothingMode::CausalEma:
                return causal_ema(px, cfg.smooth_period);
            case SmoothingMode::None:
            default:
                return px;
            }
        }

        inline bool unique_max(const std::vector<double>& s, std::size_t i, std::size_t k) {
            const double v = s[i];
            for (std::size_t d = 1; d <= k; ++d) {
                if (s[i - d] >= v || s[i + d] >= v) {
                    return false;
                }
            }
            return true;
        }

        inline bool unique_min(const std::vector<double>& s, std::size_t i, std::size_t k) {
            const double v = s[i];
            for (std::size_t d = 1; d <= k; ++d) {
                if (s[i - d] <= v || s[i + d] <= v) {
                    return false;
                }
            }
            return true;
        }


        inline std::vector<Swing> raw_extrema(std::span<const Candlestick> bars, std::size_t k, Timeframe tf) {
            std::vector<Swing> out;
            const std::size_t n = bars.size();
            if (k == 0 || n < 2 * k + 1) {
                return out;
            }
            for (std::size_t i = k; i + k < n; ++i) {
                bool is_high = true;
                bool is_low = true;
                const double h = bars[i].m_high;
                const double l = bars[i].m_low;
                for (std::size_t j = i - k; j <= i + k; ++j) {
                    if (j == i) {
                        continue;
                    }
                    if (bars[j].m_high >= h) {
                        is_high = false;
                    }
                    if (bars[j].m_low <= l) {
                        is_low = false;
                    }
                    if (!is_high && !is_low) {
                        break;
                    }
                }
                if (is_high == is_low) {
                    continue;
                }
                Swing s;
                s.index = i;
                s.is_high = is_high;
                s.price = is_high ? h : l;
                s.timeframe = tf;
                s.date = bars[i].m_date;
                s.time = bars[i].m_time;
                out.push_back(s);
            }
            return out;
        }

        inline std::vector<Swing> smoothed_extrema(std::span<const Candlestick> bars,
            const std::vector<double>& s,
            std::size_t k,
            Timeframe tf) {
            std::vector<Swing> out;
            const std::size_t n = bars.size();
            if (k == 0 || n < 2 * k + 1 || s.size() != n) {
                return out;
            }
            for (std::size_t i = k; i + k < n; ++i) {
                const bool is_high = unique_max(s, i, k);
                const bool is_low = unique_min(s, i, k);
                if (is_high == is_low) {
                    continue;
                }
                Swing sw;
                sw.index = i;
                sw.is_high = is_high;
                sw.price = is_high ? bars[i].m_high : bars[i].m_low;
                sw.timeframe = tf;
                sw.date = bars[i].m_date;
                sw.time = bars[i].m_time;
                out.push_back(sw);
            }
            return out;
        }

        inline void apply_prominence(const std::vector<Candlestick>& bars,
            std::vector<Swing>& swings,
            const SwingConfig& cfg) {
            const std::size_t k = cfg.k;
            std::vector<Swing> kept;
            kept.reserve(swings.size());
            for (auto& sw : swings) {
                const std::size_t t = sw.index;
                if (t < k || t + k >= bars.size()) {
                    continue;
                }
                double p = 0.0;
                if (sw.is_high) {
                    double l_left = bars[t - k].m_low;
                    double l_right = bars[t + k].m_low;
                    for (std::size_t d = 1; d <= k; ++d) {
                        l_left = std::min(l_left, bars[t - d].m_low);
                        l_right = std::min(l_right, bars[t + d].m_low);
                    }
                    p = std::min(bars[t].m_high - l_left, bars[t].m_high - l_right);
                }
                else {
                    double h_left = bars[t - k].m_high;
                    double h_right = bars[t + k].m_high;
                    for (std::size_t d = 1; d <= k; ++d) {
                        h_left = std::max(h_left, bars[t - d].m_high);
                        h_right = std::max(h_right, bars[t + d].m_high);
                    }
                    p = std::min(h_left - bars[t].m_low, h_right - bars[t].m_low);
                }
                p = std::max(0.0, p);
                const double a = atr_until(bars, t);
                const double pstar = (a > 0.0) ? (p / a) : (p > 0.0 ? 1e9 : 0.0);
                sw.prominence = p;
                sw.prominence_atr = pstar;
                if (pstar + 1e-12 >= cfg.prominence_atr_min) {
                    kept.push_back(sw);
                }
            }
            swings.swap(kept);
        }

        inline void apply_status(std::span<const Candlestick> bars, LiquidityZone& z, std::size_t accept) {
            if (z.source_swings.empty()) {
                return;
            }
            const std::size_t from = z.source_swings.back() + 1;
            int beyond_run = 0;
            for (std::size_t i = from; i < bars.size(); ++i) {
                const auto& c = bars[i];
                const bool in_band = c.m_high >= z.low && c.m_low <= z.high;
                bool wick_through = false;
                bool close_through = false;
                if (z.side == LiquiditySide::Buy) {
                    wick_through = c.m_high > z.high;
                    close_through = c.m_close > z.high;
                }
                else {
                    wick_through = c.m_low < z.low;
                    close_through = c.m_close < z.low;
                }

                if (!in_band && !wick_through && !close_through) {
                    beyond_run = close_through ? beyond_run : 0;
                    continue;
                }

                z.last_interaction_date = c.m_date;
                z.last_interaction_time = c.m_time;
                z.touch_indices.push_back(i);
                ++z.touch_count;

                if (close_through) {
                    ++beyond_run;
                    z.event_index = i;
                    z.status = (beyond_run >= static_cast<int>(std::max<std::size_t>(1, accept)))
                        ? ZoneStatus::Invalidated
                        : ZoneStatus::Broken;
                }
                else {
                    beyond_run = 0;
                    if (wick_through) {
                        z.event_index = i;
                        if (z.status == ZoneStatus::Active || z.status == ZoneStatus::Tested) {
                            z.status = ZoneStatus::Swept;
                        }
                    }
                    else if (z.status == ZoneStatus::Active) {
                        z.status = ZoneStatus::Tested;
                    }
                }
            }
        }

        inline void stamp_zone(LiquidityZone& z, std::span<const Candlestick> bars) {
            if (z.source_swings.empty() || z.source_swings.back() >= bars.size()) {
                return;
            }
            const auto& c = bars[z.source_swings.back()];
            z.creation_date = c.m_date;
            z.creation_time = c.m_time;
        }

    }  // namespace detail

    inline std::vector<Swing> detect_local_extrema(const std::vector<Candlestick>& bars,
        std::size_t k,
        Timeframe tf = Timeframe::M5) {
        return detail::raw_extrema(bars, k, tf);
    }

    // Full swing pipeline
    inline std::vector<Swing> detect_swings(const std::vector<Candlestick>& bars, const SwingConfig& cfg) {
        const std::size_t k = cfg.k;
        if (bars.size() < 2 * k + 1) {
            return {};
        }
        std::vector<Swing> swings;
        if (cfg.smoothing == SmoothingMode::None) {
            swings = detail::raw_extrema(bars, k, cfg.timeframe);
        }
        else {
            const auto series = detail::structure_series(bars, cfg);
            swings = detail::smoothed_extrema(bars, series, k, cfg.timeframe);
        }
        detail::apply_prominence(bars, swings, cfg);
        return swings;
    }

    // (causal SMA + ATR prominence).
    // detect_local_extrema(bars, 2) or set SwingConfig::smoothing = None and
    // prominence_atr_min = 0.
    inline std::vector<Swing> detect_swings(const std::vector<Candlestick>& bars,
        std::size_t left,
        std::size_t right) {
        SwingConfig cfg;
        cfg.k = std::max(left, right);
        cfg.smoothing = SmoothingMode::None;
        cfg.prominence_atr_min = 1.0;
        return detect_swings(bars, cfg);
    }

    inline std::vector<LiquidityZone> detect_liquidity(const std::vector<Candlestick>& bars,
        const std::vector<Swing>& swings,
        const LiquidityConfig& cfg) {
        auto last_atr = [&](const Swing& s) {
            return std::max(cfg.abs_eps, cfg.atr_epsilon_k * detail::atr_until(bars, s.index));
            };

        std::vector<Swing> highs;
        std::vector<Swing> lows;
        for (const auto& s : swings) {
            (s.is_high ? highs : lows).push_back(s);
        }

        auto cluster = [&](std::vector<Swing> side, LiquiditySide liq) {
            std::vector<LiquidityZone> zones;
            if (side.size() < cfg.min_touches) {
                return zones;
            }
            std::sort(side.begin(), side.end(), [](const Swing& a, const Swing& b) {
                if (a.price != b.price) {
                    return a.price < b.price;
                }
                return a.index < b.index;
                });
            std::size_t i = 0;
            while (i < side.size()) {
                std::size_t j = i + 1;
                double lo = side[i].price;
                double hi = side[i].price;
                double tol = last_atr(side[i]);
                while (j < side.size()) {
                    tol = std::max(tol, last_atr(side[j]));
                    if (side[j].price - hi > tol) {
                        break;
                    }
                    hi = side[j].price;
                    ++j;
                }
                if (j - i >= cfg.min_touches) {
                    const double pad = cfg.atr_pad_k * detail::atr_until(bars, side[j - 1].index);
                    LiquidityZone z;
                    z.side = liq;
                    z.low = lo - pad;
                    z.high = hi + pad;
                    z.timeframe = cfg.timeframe;
                    for (std::size_t u = i; u < j; ++u) {
                        z.source_swings.push_back(side[u].index);
                    }
                    std::sort(z.source_swings.begin(), z.source_swings.end());
                    detail::stamp_zone(z, bars);
                    detail::apply_status(bars, z, cfg.break_accept_bars);
                    zones.push_back(std::move(z));
                }
                i = (j == i) ? i + 1 : j;
            }
            return zones;
            };

        auto zones = cluster(std::move(highs), LiquiditySide::Buy);
        auto ssl = cluster(std::move(lows), LiquiditySide::Sell);
        zones.insert(zones.end(), ssl.begin(), ssl.end());
        return zones;
    }

    inline std::vector<LiquidityZone> detect_liquidity(const std::vector<Candlestick>& bars,
        const std::vector<Swing>& swings) {
        return detect_liquidity(bars, swings, LiquidityConfig{});
    }

    // One timeframe: swings then zones. Never mix H1 with M5 in one call.
    inline MarketStructure analyze_timeframe(const std::vector<Candlestick>& bars, Timeframe tf) {
        SwingConfig sc;
        sc.timeframe = tf;
        LiquidityConfig lc;
        lc.timeframe = tf;
        MarketStructure out;
        out.timeframe = tf;
        out.swings = detect_swings(bars, sc);
        out.zones = detect_liquidity(bars, out.swings, lc);
        return out;
    }

    // Importance = the TF it was detected on.
    inline std::map<Timeframe, MarketStructure> analyze_multi(
        const std::map<Timeframe, std::vector<Candlestick>>& by_tf) {
        std::map<Timeframe, MarketStructure> out;
        for (const auto& [tf, bars] : by_tf) {
            out.emplace(tf, analyze_timeframe(bars, tf));
        }
        return out;
    }

    // Recency rule: M5 stale faster than H1.
    inline std::chrono::minutes zone_relevant_horizon(Timeframe tf) {
        switch (tf) {
        case Timeframe::M1:
            return std::chrono::minutes{ 30 };
        case Timeframe::M5:
            return std::chrono::minutes{ 4 * 60 };
        case Timeframe::M15:
            return std::chrono::minutes{ 12 * 60 };
        case Timeframe::M30:
            return std::chrono::minutes{ 24 * 60 };
        case Timeframe::H1:
            return std::chrono::minutes{ 5 * 24 * 60 };
        case Timeframe::H4:
            return std::chrono::minutes{ 15 * 24 * 60 };
        default:
            return std::chrono::minutes{ 24 * 60 };
        }
    }

}  // namespace map::technical

#endif //!MOMEMNTUM_H