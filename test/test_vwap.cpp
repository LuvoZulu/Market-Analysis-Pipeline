#include "helpers.h"
#include "technical/indicators/volume_weighted_average_price.h"

#include <gtest/gtest.h>

using namespace map_test;
using map::indicators::Vwap;

TEST(M3_Vwap, TypicalPriceTimesTickVolume) {
    const auto d = make_date(2026, 7, 26);
    std::vector<Candlestick> bars{
        make_candle(d, make_tod(22, 0, 0), 10, 12, 8, 11, 1),   // typical 10.333, w=1
        make_candle(d, make_tod(22, 1, 0), 11, 14, 11, 13, 3),  // typical 12.666, w=3
    };
    Vwap vwap;
    const double expect = (typical(bars[0]) * 1.0 + typical(bars[1]) * 3.0) / 4.0;
    EXPECT_NEAR(vwap.get_average(bars), expect, 1e-9);
}

TEST(M3_Vwap, EmptyIsZero) {
    Vwap vwap;
    std::vector<Candlestick> empty;
    EXPECT_EQ(vwap.get_average(empty), 0.0);
}

TEST(M3_Vwap, DateWindowExcludesOutsideBars) {
    auto bars = std::vector<Candlestick>{
        make_candle(make_date(2026, 7, 26), make_tod(22, 0, 0), 10, 12, 8, 11, 2),
        make_candle(make_date(2026, 7, 27), make_tod(22, 0, 0), 20, 22, 18, 21, 2),
    };
    Vwap vwap;
    const auto from = make_date(2026, 7, 26);
    const auto to = make_date(2026, 7, 26);
    EXPECT_NEAR(vwap.get_average(bars, from, to), typical(bars[0]), 1e-9);
}

TEST(M3_Vwap, TickVwapIsMeanMidWhenVolumeEmpty) {
    const auto d = make_date(2026, 7, 26);
    std::vector<Tick> ticks{
        make_tick(d, make_tod(22, 1, 30, 255), 4091.771, 4092.251),
        make_tick(d, make_tod(22, 1, 30, 271), 4091.676, 4092.156),
    };
    Vwap vwap;
    const double expect = 0.5 * (mid_px(ticks[0]) + mid_px(ticks[1]));
    EXPECT_NEAR(vwap.get_average_ticks(ticks), expect, 1e-9);
}

TEST(M3_Vwap, AveragePriceToBeatSitsInsideRange) {
    const auto d = make_date(2026, 7, 26);
    std::vector<Candlestick> bars{
        make_candle(d, make_tod(22, 0, 0), 4090, 4094, 4088, 4092, 10),
        make_candle(d, make_tod(22, 1, 0), 4092, 4096, 4091, 4095, 20),
    };
    const double v = Vwap{}.get_average(bars);
    EXPECT_GE(v, 4088.0);
    EXPECT_LE(v, 4096.0);
}
