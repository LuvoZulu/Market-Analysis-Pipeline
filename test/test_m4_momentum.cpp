// M4 — later sprint. Skip lives HERE, not in src/main.cpp.
// map_tests uses GTest::gtest_main (see tests/CMakeLists.txt). Leave this
// file in add_executable so ctest reports SKIP instead of a missing test.
#include "helpers.h"

#include <gtest/gtest.h>
#include <algorithm>

using namespace map_test;

#if __has_include("technical/tools/momentum.h")
#include "technical/tools/momentum.h"
#define MAP_HAS_M4 1
#else
#define MAP_HAS_M4 0
#endif

namespace {

std::vector<Candlestick> swing_series() {
    const auto d = make_date(2026, 7, 26);
    const double ohlc[][4] = {
        {4090, 4092, 4088, 4091},
        {4091, 4093, 4090, 4092},
        {4092, 4100, 4091, 4098},  // swing high idx 2
        {4098, 4099, 4094, 4095},
        {4095, 4096, 4090, 4091},
        {4091, 4092, 4082, 4084},  // swing low idx 5
        {4084, 4097, 4084, 4096},
        {4096, 4098, 4095, 4097},
        {4097, 4108, 4096, 4106},  // swing high idx 8
        {4106, 4107, 4098, 4100},
        {4100, 4102, 4092, 4094},
        {4094, 4095, 4086, 4088},  // swing low idx 11 (HL)
        {4088, 4104, 4088, 4102},
    };
    std::vector<Candlestick> bars;
    for (int i = 0; i < 13; ++i) {
        bars.push_back(make_candle(d, make_tod(22, i, 0), ohlc[i][0], ohlc[i][1], ohlc[i][2],
                                   ohlc[i][3], 20));
    }
    return bars;
}

bool is_swing_high(const std::vector<Candlestick>& b, std::size_t i, std::size_t L, std::size_t R) {
    if (i < L || i + R >= b.size()) return false;
    for (std::size_t k = i - L; k <= i + R; ++k) {
        if (k != i && b[k].m_high >= b[i].m_high) return false;
    }
    return true;
}

bool is_swing_low(const std::vector<Candlestick>& b, std::size_t i, std::size_t L, std::size_t R) {
    if (i < L || i + R >= b.size()) return false;
    for (std::size_t k = i - L; k <= i + R; ++k) {
        if (k != i && b[k].m_low <= b[i].m_low) return false;
    }
    return true;
}

}  // namespace

TEST(M4_Momentum, OracleMarksKnownSwings) {
    const auto bars = swing_series();
    EXPECT_TRUE(is_swing_high(bars, 2, 2, 2));
    EXPECT_TRUE(is_swing_high(bars, 8, 2, 2));
    EXPECT_TRUE(is_swing_low(bars, 5, 2, 2));
    EXPECT_TRUE(is_swing_low(bars, 11, 2, 2));
    EXPECT_FALSE(is_swing_high(bars, 0, 2, 2));
}

#if MAP_HAS_M4
TEST(M4_Momentum, DetectorMatchesOracle) {
    const auto bars = swing_series();
    const auto swings = map::technical::detect_swings(bars, 2, 2);
    std::vector<std::size_t> highs, lows;
    for (const auto& s : swings) {
        (s.is_high ? highs : lows).push_back(s.index);
    }
    EXPECT_NE(std::find(highs.begin(), highs.end(), 2u), highs.end());
    EXPECT_NE(std::find(highs.begin(), highs.end(), 8u), highs.end());
    EXPECT_NE(std::find(lows.begin(), lows.end(), 5u), lows.end());
    EXPECT_NE(std::find(lows.begin(), lows.end(), 11u), lows.end());
}

TEST(M4_Momentum, LiquidityPoolsAtEqualHighs) {
    auto bars = swing_series();
    bars[2].m_high = bars[8].m_high = 4108.0;
    const auto zones = map::technical::detect_liquidity(bars, map::technical::detect_swings(bars, 2, 2));
    ASSERT_FALSE(zones.empty());
}
#else
TEST(M4_Momentum, DetectorMatchesOracle) {
    GTEST_SKIP() << "Add include/technical/tools/momentum.h (swings + liquidity)";
}

TEST(M4_Momentum, LiquidityPoolsAtEqualHighs) {
    GTEST_SKIP() << "Add include/technical/tools/momentum.h (swings + liquidity)";
}
#endif
