#pragma once

#include "map/Candlestick.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace map_test {

using map::market_data::Candlestick;
using map::market_data::Tick;
using map::market_data::Timeframe;
using ymd = std::chrono::year_month_day;
using ms = std::chrono::milliseconds;

inline ymd make_date(int y, unsigned m, unsigned d) {
    return std::chrono::year{y} / std::chrono::month{m} / std::chrono::day{d};
}

inline ms make_tod(int h, int min, int s, int millis = 0) {
    return ms{(static_cast<long long>(h) * 3600 + min * 60 + s) * 1000 + millis};
}

inline Tick make_tick(ymd date, ms tod, double bid, double ask,
                      double last = 0.0, double volume = 0.0, std::size_t flags = 6) {
    return Tick{tod, date, bid, ask, last, volume, flags};
}

inline Candlestick make_candle(ymd date, ms tod, double open, double high, double low,
                               double close, double tick_volume = 1.0, double spread = 0.0) {
    const double price = close;
    return Candlestick{tod, date, open, high, low, price, close, tick_volume, tick_volume, spread};
}

inline double mid_px(const Tick& t) {
    if (t.m_last > 0.0) return t.m_last;
    return 0.5 * (t.m_bid + t.m_ask);
}

inline bool ohlc_ok(const Candlestick& c) {
    return c.m_high >= c.m_low && c.m_high >= std::max(c.m_open, c.m_close) &&
           c.m_low <= std::min(c.m_open, c.m_close);
}

inline double typical(const Candlestick& c) {
    return (c.m_high + c.m_low + c.m_close) / 3.0;
}

inline double true_range(const Candlestick& cur, const Candlestick& prev) {
    return std::max({std::abs(cur.m_high - cur.m_low), std::abs(cur.m_high - prev.m_close),
                     std::abs(cur.m_low - prev.m_close)});
}

// Independent M1 aggregation (the contract tests lock). Mid price, tick_volume = count.
inline Candlestick aggregate_m1(const std::vector<Tick>& ticks) {
    if (ticks.empty()) return {};
    double high = -1e300;
    double low = 1e300;
    const double open = mid_px(ticks.front());
    for (const auto& t : ticks) {
        const double px = mid_px(t);
        high = std::max(high, px);
        low = std::min(low, px);
    }
    const double close = mid_px(ticks.back());
    const double spread = ticks.back().m_ask - ticks.back().m_bid;
    return Candlestick{ticks.front().m_time, ticks.front().m_date, open, high, low, close, close,
                       static_cast<double>(ticks.size()), static_cast<double>(ticks.size()), spread};
}

inline Candlestick aggregate_from_lower(const std::vector<Candlestick>& lower, std::size_t count) {
    if (lower.size() < count) return {};
    const std::size_t start = lower.size() - count;
    double high = -1e300;
    double low = 1e300;
    double tv = 0.0;
    for (std::size_t i = start; i < lower.size(); ++i) {
        high = std::max(high, lower[i].m_high);
        low = std::min(low, lower[i].m_low);
        tv += lower[i].m_tick_volume;
    }
    const auto& o = lower[start];
    const auto& c = lower.back();
    return Candlestick{o.m_time, o.m_date, o.m_open, high, low, c.m_close, c.m_close, tv, tv,
                       c.m_spread};
}

}  // namespace map_test
