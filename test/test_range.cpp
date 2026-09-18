#include "helpers.h"

#include <gtest/gtest.h>

using namespace map_test;

// LUV-10 / ISSUES.md #5: average range of a pair between two timestamps.
// Contract lives here until you add include/technical/indicators/range.h

namespace {

double avg_range(const std::vector<Candlestick>& bars, ymd from_d, ms from_t, ymd to_d, ms to_t) {
    double acc = 0.0;
    std::size_t n = 0;
    auto in = [&](const Candlestick& c) {
        if (c.m_date < from_d) return false;
        if (c.m_date == from_d && c.m_time < from_t) return false;
        if (c.m_date > to_d) return false;
        if (c.m_date == to_d && c.m_time > to_t) return false;
        return true;
    };
    for (const auto& c : bars) {
        if (!in(c)) continue;
        acc += (c.m_high - c.m_low);
        ++n;
    }
    return n ? acc / static_cast<double>(n) : 0.0;
}

}  // namespace

TEST(M3_Range, AverageBetweenTimesOnSameDay) {
    const auto d = make_date(2026, 7, 26);
    std::vector<Candlestick> bars{
        make_candle(d, make_tod(22, 0, 0), 10, 12, 9, 11, 1),    // range 3
        make_candle(d, make_tod(22, 1, 0), 11, 14, 10, 13, 1),   // range 4
        make_candle(d, make_tod(22, 2, 0), 13, 20, 13, 19, 1),   // range 7, excluded
    };
    const double got = avg_range(bars, d, make_tod(22, 0, 0), d, make_tod(22, 1, 0));
    EXPECT_NEAR(got, 3.5, 1e-12);
}

TEST(M3_Range, EmptyWindowIsZero) {
    const auto d = make_date(2026, 7, 26);
    std::vector<Candlestick> bars{make_candle(d, make_tod(22, 0, 0), 10, 12, 9, 11, 1)};
    EXPECT_EQ(avg_range(bars, d, make_tod(23, 0, 0), d, make_tod(23, 59, 0)), 0.0);
}

#if __has_include("technical/indicators/range.h")
#include "technical/indicators/range.h"
TEST(M3_Range, ProductionMatchesContract) {
    const auto d = make_date(2026, 7, 26);
    std::vector<Candlestick> bars{
        make_candle(d, make_tod(22, 0, 0), 10, 12, 9, 11, 1),
        make_candle(d, make_tod(22, 1, 0), 11, 14, 10, 13, 1),
    };
    auto from_t = make_tod(22, 0, 0);
    auto to_t = make_tod(22, 1, 0);
    auto day = d;
    EXPECT_NEAR(map::indicators::Range{}.get_average(bars, day, from_t, day, to_t), 3.5, 1e-12);
}
#endif
