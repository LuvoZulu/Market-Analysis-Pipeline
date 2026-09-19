#pragma once

// M5 — pure TA structure engine (spec: objective → derived → events).
// No ML, probabilities, weights, or regression.
// Depends on momentum.h (swings, liquidity) and map::indicators::Atr.

#include "technical/tools/momentum.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace map::technical {

    using map::market_data::Candlestick;
    using map::market_data::Timeframe;


    enum class Trend { Bullish, Bearish, Transition, Ranging };

    struct TrendState {
        Trend trend{ Trend::Ranging };
        std::optional<Swing> last_swing_high;
        std::optional<Swing> previous_swing_high;
        std::optional<Swing> last_swing_low;
        std::optional<Swing> previous_swing_low;
        bool hh{ false };
        bool hl{ false };
        bool lh{ false };
        bool ll{ false };
        bool ma_agrees{ false };  // secondary only; never overrides structure
    };

    struct TrendConfig {
        std::size_t ma_period{ 0 };  // 0 = skip MA confirmation
        Timeframe timeframe{ Timeframe::M5 };
    };

    enum class SessionId { Asia, London, NewYork, LondonNewYorkOverlap, OffSession };

    struct SessionConfig {
        // Minutes from midnight in the candle clock (treat m_time as that clock).
        // Defaults ≈ UTC winter FX hours. DST: set IANA names and use_iana_tz.
        int asia_open{ 0 };
        int asia_close{ 8 * 60 };
        int london_open{ 7 * 60 };
        int london_close{ 16 * 60 };
        int ny_open{ 12 * 60 };
        int ny_close{ 21 * 60 };
        bool use_iana_tz{ false };
        const char* london_tz{ "Europe/London" };
        const char* ny_tz{ "America/New_York" };
        const char* asia_tz{ "Asia/Tokyo" };
        int london_local_open_min{ 8 * 60 };
        int london_local_close_min{ 16 * 60 + 30 };
        int ny_local_open_min{ 8 * 60 };
        int ny_local_close_min{ 17 * 60 };
        int asia_local_open_min{ 9 * 60 };
        int asia_local_close_min{ 18 * 60 };
    };

    struct SessionSnapshot {
        SessionId id{ SessionId::OffSession };
        std::size_t start_index{};
        std::size_t end_index{};
        double open{};
        double high{};
        double low{};
        double close{};
        double range{};
        std::chrono::year_month_day date{};
        std::chrono::milliseconds open_time{};
        std::chrono::milliseconds close_time{};
    };

    struct SessionTag {
        SessionId id{ SessionId::OffSession };
        double session_open{};
        double session_high{};
        double session_low{};
        double session_range{};
    };

    // ---------------------------------------------------------------------------
    // 3. S/R  4. OB  5. FVG  6–8. BOS / CHoCH
    // ---------------------------------------------------------------------------

    enum class SrKind { Support, Resistance };

    struct SrZone {
        SrKind kind{};
        double low{};
        double high{};
        Timeframe timeframe{ Timeframe::M5 };
        std::vector<std::size_t> sources;
        int touch_count{ 0 };
    };

    struct FairValueGap {
        bool is_bullish{};
        double bottom{};
        double top{};
        std::size_t c1{};
        std::size_t c2{};  // displacement candle
        std::size_t c3{};
        bool mitigated{ false };
        std::size_t mitigate_index{ static_cast<std::size_t>(-1) };
    };

    enum class StructureEventKind { Bos, Choch, Sweep };

    struct StructureEvent {
        StructureEventKind kind{};
        bool is_bullish{};
        std::size_t bar{};
        double level{};
        std::size_t broken_swing{};
        Timeframe timeframe{ Timeframe::M5 };
    };

    struct OrderBlock {
        bool is_bullish{};
        double low{};
        double high{};
        std::size_t candle{};
        std::size_t bos_bar{};
        Timeframe timeframe{ Timeframe::M5 };
        bool mitigated{ false };
    };

    struct FvgConfig {
        double min_gap_atr{ 0.0 };          // 0 = any geometric gap (GTest)
        double displacement_atr_min{ 0.0 }; // 0 = do not require C2 size
        bool require_bos_or_choch{ false };
    };

    struct DisplacementConfig {
        double atr_min{ 1.0 };  // |body| or range vs ATR
    };

    struct StructureConfig {
        SwingConfig swings{};
        LiquidityConfig liquidity{};
        TrendConfig trend{};
        SessionConfig sessions{};
        FvgConfig fvg{};
        DisplacementConfig displacement{};
        double sr_atr_eps{ 0.10 };
        Timeframe timeframe{ Timeframe::M5 };
    };

    struct EngineReport {
        Timeframe timeframe{ Timeframe::M5 };
        std::vector<SessionTag> bar_session;
        std::vector<SessionSnapshot> sessions;
        std::vector<Swing> swings;
        TrendState trend;
        std::vector<SrZone> support_resistance;
        std::vector<LiquidityZone> liquidity;
        std::vector<FairValueGap> fvgs;
        std::vector<StructureEvent> events;
        std::vector<OrderBlock> order_blocks;
    };

    // ---------------------------------------------------------------------------
    // detail
    // ---------------------------------------------------------------------------

    namespace detail {

        inline bool bearish_candle(const Candlestick& c) { return c.m_close < c.m_open; }
        inline bool bullish_candle(const Candlestick& c) { return c.m_close > c.m_open; }
        inline double body(const Candlestick& c) { return std::abs(c.m_close - c.m_open); }
        inline double range(const Candlestick& c) { return c.m_high - c.m_low; }

        inline int tod_minutes(const Candlestick& c) {
            return static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(c.m_time).count());
        }

        inline bool in_window(int m, int open, int close) {
            if (open == close) {
                return false;
            }
            if (open < close) {
                return m >= open && m < close;
            }
            return m >= open || m < close;  // wraps midnight
        }

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
        inline std::optional<int> local_minutes_iana(const Candlestick& c, const char* tz_name) {
            try {
                const auto* tz = std::chrono::locate_zone(tz_name);
                const auto utc = std::chrono::sys_days{ c.m_date } + c.m_time;
                const auto loc = tz->to_local(std::chrono::time_point_cast<std::chrono::milliseconds>(utc));
                const auto dp = std::chrono::floor<std::chrono::days>(loc);
                const auto tod = loc - dp;
                return static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(tod).count());
            }
            catch (...) {
                return std::nullopt;
            }
        }
#endif

        inline SessionId classify_session(const Candlestick& c, const SessionConfig& cfg) {
            if (cfg.use_iana_tz) {
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
                int asia_m = tod_minutes(c);
                int lon_m = asia_m;
                int ny_m = asia_m;
                if (auto a = local_minutes_iana(c, cfg.asia_tz)) {
                    asia_m = *a;
                }
                if (auto a = local_minutes_iana(c, cfg.london_tz)) {
                    lon_m = *a;
                }
                if (auto a = local_minutes_iana(c, cfg.ny_tz)) {
                    ny_m = *a;
                }
                const bool asia = in_window(asia_m, cfg.asia_local_open_min, cfg.asia_local_close_min);
                const bool lon = in_window(lon_m, cfg.london_local_open_min, cfg.london_local_close_min);
                const bool ny = in_window(ny_m, cfg.ny_local_open_min, cfg.ny_local_close_min);
                if (lon && ny) {
                    return SessionId::LondonNewYorkOverlap;
                }
                if (ny) {
                    return SessionId::NewYork;
                }
                if (lon) {
                    return SessionId::London;
                }
                if (asia) {
                    return SessionId::Asia;
                }
                return SessionId::OffSession;
#endif
            }
            const bool asia = in_window(tod_minutes(c), cfg.asia_open, cfg.asia_close);
            const bool lon = in_window(tod_minutes(c), cfg.london_open, cfg.london_close);
            const bool ny = in_window(tod_minutes(c), cfg.ny_open, cfg.ny_close);
            if (lon && ny) {
                return SessionId::LondonNewYorkOverlap;
            }
            if (ny) {
                return SessionId::NewYork;
            }
            if (lon) {
                return SessionId::London;
            }
            if (asia) {
                return SessionId::Asia;
            }
            return SessionId::OffSession;
        }

        inline double sma_close(const std::vector<Candlestick>& bars, std::size_t end_inclusive, std::size_t period) {
            if (period == 0 || bars.empty()) {
                return 0.0;
            }
            const std::size_t n = std::min(end_inclusive + 1, bars.size());
            const std::size_t from = n > period ? n - period : 0;
            double s = 0.0;
            std::size_t c = 0;
            for (std::size_t i = from; i < n; ++i) {
                s += bars[i].m_close;
                ++c;
            }
            return c ? s / static_cast<double>(c) : 0.0;
        }

        inline bool displacement(const Candlestick& c, double atr, const DisplacementConfig& cfg, bool want_bull) {
            if (atr <= 0.0) {
                return body(c) > 0.0;
            }
            const bool dir = want_bull ? bullish_candle(c) : bearish_candle(c);
            return dir && (range(c) + 1e-12 >= cfg.atr_min * atr || body(c) + 1e-12 >= cfg.atr_min * atr);
        }

    }  // namespace detail

    // ---------------------------------------------------------------------------
    // Public detectors
    // ---------------------------------------------------------------------------

    inline SessionId session_of(const Candlestick& c, const SessionConfig& cfg = {}) {
        return detail::classify_session(c, cfg);
    }

    inline std::vector<SessionTag> tag_sessions(const std::vector<Candlestick>& bars,
        const SessionConfig& cfg = {}) {
        std::vector<SessionTag> out(bars.size());
        SessionId cur = SessionId::OffSession;
        double o = 0, h = 0, l = 0;
        for (std::size_t i = 0; i < bars.size(); ++i) {
            const SessionId id = detail::classify_session(bars[i], cfg);
            if (i == 0 || id != cur) {
                cur = id;
                o = bars[i].m_open;
                h = bars[i].m_high;
                l = bars[i].m_low;
            }
            else {
                h = std::max(h, bars[i].m_high);
                l = std::min(l, bars[i].m_low);
            }
            out[i].id = id;
            out[i].session_open = o;
            out[i].session_high = h;
            out[i].session_low = l;
            out[i].session_range = h - l;
        }
        return out;
    }

    inline std::vector<SessionSnapshot> detect_sessions(const std::vector<Candlestick>& bars,
        const SessionConfig& cfg = {}) {
        std::vector<SessionSnapshot> out;
        if (bars.empty()) {
            return out;
        }
        SessionSnapshot cur;
        cur.id = detail::classify_session(bars[0], cfg);
        cur.start_index = 0;
        cur.open = bars[0].m_open;
        cur.high = bars[0].m_high;
        cur.low = bars[0].m_low;
        cur.date = bars[0].m_date;
        cur.open_time = bars[0].m_time;
        for (std::size_t i = 1; i <= bars.size(); ++i) {
            const bool last = i == bars.size();
            const SessionId id = last ? cur.id : detail::classify_session(bars[i], cfg);
            const bool new_sess = last || id != cur.id || bars[i].m_date != cur.date;
            if (new_sess) {
                cur.end_index = i - 1;
                cur.close = bars[cur.end_index].m_close;
                cur.close_time = bars[cur.end_index].m_time;
                cur.range = cur.high - cur.low;
                out.push_back(cur);
                if (last) {
                    break;
                }
                cur = SessionSnapshot{};
                cur.id = id;
                cur.start_index = i;
                cur.open = bars[i].m_open;
                cur.high = bars[i].m_high;
                cur.low = bars[i].m_low;
                cur.date = bars[i].m_date;
                cur.open_time = bars[i].m_time;
            }
            else {
                cur.high = std::max(cur.high, bars[i].m_high);
                cur.low = std::min(cur.low, bars[i].m_low);
            }
        }
        return out;
    }

    inline TrendState detect_trend(const std::vector<Swing>& swings,
        const std::vector<Candlestick>& bars = {},
        const TrendConfig& cfg = {}) {
        TrendState st;
        std::vector<Swing> highs, lows;
        for (const auto& s : swings) {
            (s.is_high ? highs : lows).push_back(s);
        }
        if (highs.size() >= 2) {
            st.previous_swing_high = highs[highs.size() - 2];
            st.last_swing_high = highs.back();
            st.hh = st.last_swing_high->price > st.previous_swing_high->price;
            st.lh = st.last_swing_high->price < st.previous_swing_high->price;
        }
        if (lows.size() >= 2) {
            st.previous_swing_low = lows[lows.size() - 2];
            st.last_swing_low = lows.back();
            st.hl = st.last_swing_low->price > st.previous_swing_low->price;
            st.ll = st.last_swing_low->price < st.previous_swing_low->price;
        }
        if (st.hh && st.hl) {
            st.trend = Trend::Bullish;
        }
        else if (st.lh && st.ll) {
            st.trend = Trend::Bearish;
        }
        else if ((st.hh && st.ll) || (st.lh && st.hl)) {
            st.trend = Trend::Transition;
        }
        else {
            st.trend = Trend::Ranging;
        }

        if (cfg.ma_period > 0 && bars.size() >= 2) {
            const double ma_now = detail::sma_close(bars, bars.size() - 1, cfg.ma_period);
            const double ma_prev = detail::sma_close(bars, bars.size() - 2, cfg.ma_period);
            const double px = bars.back().m_close;
            const bool slope_up = ma_now > ma_prev;
            const bool slope_dn = ma_now < ma_prev;
            if (st.trend == Trend::Bullish) {
                st.ma_agrees = px > ma_now && slope_up;
            }
            else if (st.trend == Trend::Bearish) {
                st.ma_agrees = px < ma_now && slope_dn;
            }
            // Structure still wins: MA never flips trend.
        }
        return st;
    }

    inline std::vector<FairValueGap> detect_fvgs(const std::vector<Candlestick>& bars,
        const FvgConfig& cfg = {}) {
        std::vector<FairValueGap> out;
        if (bars.size() < 3) {
            return out;
        }
        for (std::size_t i = 0; i + 2 < bars.size(); ++i) {
            const auto& c1 = bars[i];
            const auto& c2 = bars[i + 1];
            const auto& c3 = bars[i + 2];
            FairValueGap g;
            g.c1 = i;
            g.c2 = i + 1;
            g.c3 = i + 2;
            if (c3.m_low > c1.m_high) {
                g.is_bullish = true;
                g.bottom = c1.m_high;
                g.top = c3.m_low;
            }
            else if (c3.m_high < c1.m_low) {
                g.is_bullish = false;
                g.bottom = c3.m_high;
                g.top = c1.m_low;
            }
            else {
                continue;
            }
            const double gap = g.top - g.bottom;
            const double atr = detail::atr_until(bars, i + 2);
            if (cfg.min_gap_atr > 0.0 && atr > 0.0 && gap + 1e-12 < cfg.min_gap_atr * atr) {
                continue;
            }
            if (cfg.displacement_atr_min > 0.0 &&
                !detail::displacement(c2, atr, DisplacementConfig{ cfg.displacement_atr_min }, g.is_bullish)) {
                continue;
            }
            for (std::size_t j = i + 3; j < bars.size(); ++j) {
                const bool fill = g.is_bullish ? (bars[j].m_low <= g.bottom) : (bars[j].m_high >= g.top);
                if (fill) {
                    g.mitigated = true;
                    g.mitigate_index = j;
                    break;
                }
            }
            out.push_back(g);
        }
        return out;
    }

    inline std::vector<SrZone> detect_support_resistance(const std::vector<Candlestick>& bars,
        const std::vector<Swing>& swings,
        const std::vector<SessionSnapshot>& sessions,
        double eps_atr_k = 0.10) {
        struct Pt {
            double px;
            std::size_t idx;
            bool is_res;
        };
        std::vector<Pt> pts;
        for (const auto& s : swings) {
            pts.push_back(Pt{ s.price, s.index, s.is_high });
        }
        for (const auto& sess : sessions) {
            if (sess.id == SessionId::OffSession) {
                continue;
            }
            pts.push_back(Pt{ sess.high, sess.end_index, true });
            pts.push_back(Pt{ sess.low, sess.end_index, false });
        }
        std::vector<SrZone> zones;
        auto cluster = [&](bool res) {
            std::vector<Pt> side;
            for (const auto& p : pts) {
                if (p.is_res == res) {
                    side.push_back(p);
                }
            }
            std::sort(side.begin(), side.end(), [](const Pt& a, const Pt& b) { return a.px < b.px; });
            std::size_t i = 0;
            while (i < side.size()) {
                const double atr = detail::atr_until(bars, side[i].idx);
                const double tol = std::max(1e-9, eps_atr_k * atr);
                std::size_t j = i + 1;
                double lo = side[i].px;
                double hi = side[i].px;
                while (j < side.size() && side[j].px - hi <= tol) {
                    hi = side[j].px;
                    ++j;
                }
                if (j - i >= 1) {
                    SrZone z;
                    z.kind = res ? SrKind::Resistance : SrKind::Support;
                    z.low = lo;
                    z.high = hi;
                    for (std::size_t u = i; u < j; ++u) {
                        z.sources.push_back(side[u].idx);
                    }
                    z.touch_count = static_cast<int>(z.sources.size());
                    zones.push_back(std::move(z));
                }
                i = j;
            }
            };
        cluster(true);
        cluster(false);
        return zones;
    }

    inline std::vector<StructureEvent> detect_bos_choch(const std::vector<Candlestick>& bars,
        const std::vector<Swing>& swings,
        const TrendState& trend,
        Timeframe tf = Timeframe::M5) {
        std::vector<StructureEvent> out;
        if (swings.empty() || bars.empty()) {
            return out;
        }

        const Swing* last_sh = nullptr;
        const Swing* last_sl = nullptr;
        for (const auto& s : swings) {
            if (s.is_high) {
                last_sh = &s;
            }
            else {
                last_sl = &s;
            }
        }

        auto close_beyond_high = [&](std::size_t from, double lvl) -> std::optional<std::size_t> {
            for (std::size_t i = from; i < bars.size(); ++i) {
                if (bars[i].m_close > lvl) {
                    return i;
                }
            }
            return std::nullopt;
            };
        auto close_beyond_low = [&](std::size_t from, double lvl) -> std::optional<std::size_t> {
            for (std::size_t i = from; i < bars.size(); ++i) {
                if (bars[i].m_close < lvl) {
                    return i;
                }
            }
            return std::nullopt;
            };
        auto wick_beyond_high = [&](std::size_t from, double lvl) -> std::optional<std::size_t> {
            for (std::size_t i = from; i < bars.size(); ++i) {
                if (bars[i].m_high > lvl && bars[i].m_close <= lvl) {
                    return i;
                }
            }
            return std::nullopt;
            };
        auto wick_beyond_low = [&](std::size_t from, double lvl) -> std::optional<std::size_t> {
            for (std::size_t i = from; i < bars.size(); ++i) {
                if (bars[i].m_low < lvl && bars[i].m_close >= lvl) {
                    return i;
                }
            }
            return std::nullopt;
            };

        const auto emit = [&](StructureEventKind k, bool bull, std::size_t bar, double lvl, std::size_t sw) {
            out.push_back(StructureEvent{ k, bull, bar, lvl, sw, tf });
            };

        // Sweeps: wick through confirmed swing, close back (not BOS).
        if (last_sh) {
            if (auto w = wick_beyond_high(last_sh->index + 1, last_sh->price)) {
                emit(StructureEventKind::Sweep, true, *w, last_sh->price, last_sh->index);
            }
        }
        if (last_sl) {
            if (auto w = wick_beyond_low(last_sl->index + 1, last_sl->price)) {
                emit(StructureEventKind::Sweep, false, *w, last_sl->price, last_sl->index);
            }
        }

        if (trend.trend == Trend::Bullish) {
            if (last_sh) {
                if (auto b = close_beyond_high(last_sh->index + 1, last_sh->price)) {
                    emit(StructureEventKind::Bos, true, *b, last_sh->price, last_sh->index);
                }
            }
            if (last_sl) {
                if (auto b = close_beyond_low(last_sl->index + 1, last_sl->price)) {
                    emit(StructureEventKind::Choch, false, *b, last_sl->price, last_sl->index);
                }
            }
        }
        else if (trend.trend == Trend::Bearish) {
            if (last_sl) {
                if (auto b = close_beyond_low(last_sl->index + 1, last_sl->price)) {
                    emit(StructureEventKind::Bos, false, *b, last_sl->price, last_sl->index);
                }
            }
            if (last_sh) {
                if (auto b = close_beyond_high(last_sh->index + 1, last_sh->price)) {
                    emit(StructureEventKind::Choch, true, *b, last_sh->price, last_sh->index);
                }
            }
        }
        else {
            // No forced BOS in transition/range; close through still recorded as CHoCH
            // only when both a high and a low exist (character can change).
            if (last_sh) {
                if (auto b = close_beyond_high(last_sh->index + 1, last_sh->price)) {
                    emit(StructureEventKind::Choch, true, *b, last_sh->price, last_sh->index);
                }
            }
            if (last_sl) {
                if (auto b = close_beyond_low(last_sl->index + 1, last_sl->price)) {
                    emit(StructureEventKind::Choch, false, *b, last_sl->price, last_sl->index);
                }
            }
        }
        return out;
    }

    inline std::vector<OrderBlock> detect_order_blocks(const std::vector<Candlestick>& bars,
        const std::vector<StructureEvent>& events,
        const DisplacementConfig& dcfg = {},
        Timeframe tf = Timeframe::M5) {
        std::vector<OrderBlock> out;
        for (const auto& ev : events) {
            if (ev.kind != StructureEventKind::Bos) {
                continue;
            }
            const double atr = detail::atr_until(bars, ev.bar);
            if (ev.bar >= bars.size() ||
                !detail::displacement(bars[ev.bar], atr, dcfg, ev.is_bullish)) {
                // Allow the BOS bar itself or the run into it.
                bool run = false;
                if (ev.bar > 0) {
                    run = detail::displacement(bars[ev.bar - 1], atr, dcfg, ev.is_bullish);
                }
                if (!run && !detail::displacement(bars[ev.bar], atr, DisplacementConfig{ 0.0 }, ev.is_bullish)) {
                    continue;
                }
            }
            // Last opposite candle before the BOS bar.
            std::optional<std::size_t> ob;
            for (std::size_t i = ev.bar; i > 0; --i) {
                const std::size_t j = i - 1;
                const bool opp = ev.is_bullish ? detail::bearish_candle(bars[j])
                    : detail::bullish_candle(bars[j]);
                if (opp) {
                    ob = j;
                    break;
                }
            }
            if (!ob) {
                continue;
            }
            const auto& c = bars[*ob];
            OrderBlock b;
            b.is_bullish = ev.is_bullish;
            b.low = std::min(c.m_open, c.m_close);
            b.high = std::max(c.m_open, c.m_close);
            b.candle = *ob;
            b.bos_bar = ev.bar;
            b.timeframe = tf;
            for (std::size_t k = ev.bar + 1; k < bars.size(); ++k) {
                const bool mit = b.is_bullish ? (bars[k].m_low <= b.low) : (bars[k].m_high >= b.high);
                if (mit) {
                    b.mitigated = true;
                    break;
                }
            }
            out.push_back(b);
        }
        return out;
    }

    inline EngineReport analyze_structure(const std::vector<Candlestick>& bars, const StructureConfig& cfg = {}) {
        EngineReport r;
        r.timeframe = cfg.timeframe;
        r.bar_session = tag_sessions(bars, cfg.sessions);
        r.sessions = detect_sessions(bars, cfg.sessions);

        SwingConfig sc = cfg.swings;
        sc.timeframe = cfg.timeframe;
        r.swings = detect_swings(bars, sc);
        r.trend = detect_trend(r.swings, bars, cfg.trend);

        LiquidityConfig lc = cfg.liquidity;
        lc.timeframe = cfg.timeframe;
        r.liquidity = detect_liquidity(bars, r.swings, lc);

        r.support_resistance = detect_support_resistance(bars, r.swings, r.sessions, cfg.sr_atr_eps);
        r.fvgs = detect_fvgs(bars, cfg.fvg);
        r.events = detect_bos_choch(bars, r.swings, r.trend, cfg.timeframe);
        r.order_blocks = detect_order_blocks(bars, r.events, cfg.displacement, cfg.timeframe);

        if (cfg.fvg.require_bos_or_choch) {
            std::vector<FairValueGap> kept;
            for (const auto& g : r.fvgs) {
                bool near = false;
                for (const auto& e : r.events) {
                    if (e.kind == StructureEventKind::Sweep) {
                        continue;
                    }
                    if (e.bar + 2 >= g.c1 && e.bar <= g.c3 + 2) {
                        near = true;
                        break;
                    }
                }
                if (near) {
                    kept.push_back(g);
                }
            }
            r.fvgs.swap(kept);
        }
        return r;
    }

    inline EngineReport analyze_structure(const std::vector<Candlestick>& bars, Timeframe tf) {
        StructureConfig cfg;
        cfg.timeframe = tf;
        cfg.swings.timeframe = tf;
        cfg.liquidity.timeframe = tf;
        cfg.trend.timeframe = tf;
        return analyze_structure(bars, cfg);
    }

}  // namespace map::technical