#include "helpers.h"
#include "technical/indicators/average_true_range.h"

#include <gtest/gtest.h>

using namespace map_test;
using map::indicators::Atr;

namespace {

    std::vector<Candlestick> two_day_bars() {
        const auto d0 = make_date(2026, 7, 26);
        const auto d1 = make_date(2026, 7, 27);
        return {
            make_candle(d0, make_tod(22, 0, 0), 4090, 4092, 4088, 4091, 10, 0.4),
            make_candle(d0, make_tod(22, 1, 0), 4091, 4095, 4090, 4094, 12, 0.4),
            make_candle(d1, make_tod(22, 0, 0), 4094, 4096, 4091, 4092, 11, 0.4),
            make_candle(d1, make_tod(22, 1, 0), 4092, 4093, 4087, 4088, 9, 0.4),
        };
    }

}  // namespace

TEST(M3_Atr, EmptyAndSingleBarAreZero) {
    Atr atr;
    std::vector<Candlestick> empty;
    auto day = make_date(2026, 7, 26);
    EXPECT_EQ(atr.get_average(empty, day), 0.0);

    std::vector<Candlestick> one{ make_candle(day, make_tod(22, 0, 0), 1, 2, 0.5, 1.2, 1) };
    EXPECT_EQ(atr.get_average(one, day), 0.0);
}

TEST(M3_Atr, TrueRangeMatchesTextbook) {
    // TR = max(H-L, |H-prevC|, |L-prevC|)
    const auto d = make_date(2026, 7, 26);
    std::vector<Candlestick> bars{
        make_candle(d, make_tod(22, 0, 0), 10, 12, 9, 11, 1),
        make_candle(d, make_tod(22, 1, 0), 11, 15, 10, 14, 1),
    };
    // TR = max(5, |15-11|, |10-11|) = max(5, 4, 1) = 5
    EXPECT_DOUBLE_EQ(true_range(bars[1], bars[0]), 5.0);
}

TEST(M3_Atr, DateWindowIncludesTheTargetDay) {
    // Current Atr::get_average(data, target_date) stops WHEN it sees the date,
    // so a same-day series returns 0. That is ISSUES.md #5. The contract is:
    // include every bar whose m_date == target_date (and the previous close
    // for the first TR on that day).
    auto bars = two_day_bars();
    auto day = make_date(2026, 7, 27);
    Atr atr;
    const double got = atr.get_average(bars, day);

    const double tr2 = true_range(bars[2], bars[1]);
    const double tr3 = true_range(bars[3], bars[2]);
    const double expect = 0.5 * (tr2 + tr3);
    EXPECT_NEAR(got, expect, 1e-9) << "same-session ATR must not be 0";
}

TEST(M3_Atr, InclusiveDateRange) {
    auto bars = two_day_bars();
    auto from = make_date(2026, 7, 26);
    auto to = make_date(2026, 7, 27);
    Atr atr;
    const double got = atr.get_average(bars, from, to);
    const double expect =
        (true_range(bars[1], bars[0]) + true_range(bars[2], bars[1]) + true_range(bars[3], bars[2])) /
        3.0;
    EXPECT_NEAR(got, expect, 1e-9);
}

TEST(M3_Atr, DateTimeWindowUsesFromTimeNotToTime) {
    auto bars = two_day_bars();
    auto from_d = make_date(2026, 7, 26);
    auto to_d = make_date(2026, 7, 26);
    auto from_t = make_tod(22, 1, 0);
    auto to_t = make_tod(22, 1, 0);
    Atr atr;
    // Fourth overload currently compares m_time to to_time on the skip path
    // (ISSUES.md #5). Contract: [from, to] on the same day includes bar[1] TR.
    const double got = atr.get_average(bars, from_d, from_t, to_d, to_t);
    EXPECT_GT(got, 0.0);
}

TEST(M3_Atr, GoldSessionRangeIsPositive) {
    const auto d = make_date(2026, 7, 26);
    std::vector<Candlestick> bars;
    double px = 4092.0;
    for (int i = 0; i < 10; ++i) {
        bars.push_back(make_candle(d, make_tod(22, 1, i), px, px + 1.5, px - 1.0, px + 0.3, 5, 0.48));
        px += 0.3;
    }
    Atr atr;
    auto day = d;
    EXPECT_GT(atr.get_average(bars, day), 0.0);
}
