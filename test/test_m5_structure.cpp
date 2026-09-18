// M5 — later sprint. Skip lives HERE, not in src/main.cpp.
#include "helpers.h"

#include <gtest/gtest.h>

using namespace map_test;

#if __has_include("technical/tools/market_structure.h")
#include "technical/tools/market_structure.h"
#define MAP_HAS_M5 1
#else
#define MAP_HAS_M5 0
#endif

TEST(M5_Fvg, BullishGapWhenCandle0HighBelowCandle2Low) {
    const auto d = make_date(2026, 7, 26);
    std::vector<Candlestick> bars{
        make_candle(d, make_tod(22, 0, 0), 4090, 4091, 4088, 4090.5, 10),
        make_candle(d, make_tod(22, 1, 0), 4090.5, 4098, 4090.4, 4097, 10),
        make_candle(d, make_tod(22, 2, 0), 4097, 4100, 4093, 4099, 10),
    };
    EXPECT_LT(bars[0].m_high, bars[2].m_low);
#if MAP_HAS_M5
    const auto fvgs = map::technical::detect_fvgs(bars);
    ASSERT_FALSE(fvgs.empty());
    EXPECT_NEAR(fvgs.front().bottom, bars[0].m_high, 1e-9);
    EXPECT_NEAR(fvgs.front().top, bars[2].m_low, 1e-9);
#else
    GTEST_SKIP() << "Add include/technical/tools/market_structure.h";
#endif
}

TEST(M5_Fvg, BearishGapWhenCandle0LowAboveCandle2High) {
    const auto d = make_date(2026, 7, 26);
    std::vector<Candlestick> bars{
        make_candle(d, make_tod(22, 0, 0), 4100, 4102, 4098, 4099, 10),
        make_candle(d, make_tod(22, 1, 0), 4099, 4099.2, 4090, 4091, 10),
        make_candle(d, make_tod(22, 2, 0), 4091, 4094, 4088, 4090, 10),
    };
    EXPECT_GT(bars[0].m_low, bars[2].m_high);
#if MAP_HAS_M5
    const auto fvgs = map::technical::detect_fvgs(bars);
    ASSERT_FALSE(fvgs.empty());
#else
    GTEST_SKIP() << "Add include/technical/tools/market_structure.h";
#endif
}

TEST(M5_Structure, BosAndOrderBlockNeedTheHeader) {
#if MAP_HAS_M5
    SUCCEED();
#else
    GTEST_SKIP() << "M5: FVG, order blocks, CHoCH/BOS, S/R — technical/tools/market_structure.h";
#endif
}
