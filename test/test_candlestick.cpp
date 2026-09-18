#include "helpers.h"

#include <gtest/gtest.h>

using namespace map_test;
using map::market_data::CandleStickBuilder;
using map::market_data::Candlestick;
using map::market_data::Tick;
using map::market_data::Timeframe;

TEST(M1_Candlestick, DefaultIsZeroed) {
    Candlestick c;
    EXPECT_EQ(c.m_open, 0.0);
    EXPECT_EQ(c.m_high, 0.0);
    EXPECT_EQ(c.m_low, 0.0);
    EXPECT_EQ(c.m_close, 0.0);
    EXPECT_EQ(c.m_tick_volume, 0.0);
}

TEST(M1_Candlestick, ConstructorArgumentOrder) {
    const auto day = make_date(2026, 7, 26);
    const auto tod = make_tod(22, 1, 30, 255);
    Candlestick c{tod, day, 4091.0, 4093.0, 4090.0, 4092.0, 4092.5, 12.0, 12.0, 0.48};
    EXPECT_EQ(c.m_date, day);
    EXPECT_EQ(c.m_time, tod);
    EXPECT_DOUBLE_EQ(c.m_open, 4091.0);
    EXPECT_DOUBLE_EQ(c.m_high, 4093.0);
    EXPECT_DOUBLE_EQ(c.m_low, 4090.0);
    EXPECT_DOUBLE_EQ(c.m_price, 4092.0);
    EXPECT_DOUBLE_EQ(c.m_close, 4092.5);
    EXPECT_DOUBLE_EQ(c.m_tick_volume, 12.0);
    EXPECT_DOUBLE_EQ(c.m_spread, 0.48);
    EXPECT_TRUE(ohlc_ok(c));
}

TEST(M1_Tick, MidPrefersLastOtherwiseBidAsk) {
    auto t = make_tick(make_date(2026, 7, 26), make_tod(22, 1, 30), 4091.771, 4092.251);
    EXPECT_NEAR(mid_px(t), 0.5 * (4091.771 + 4092.251), 1e-9);
    t.m_last = 4092.000;
    EXPECT_NEAR(mid_px(t), 4092.000, 1e-9);
}

TEST(M1_Tick, SpreadIsAskMinusBid) {
    const auto t = make_tick(make_date(2026, 7, 26), make_tod(22, 1, 30), 4091.771, 4092.251);
    EXPECT_NEAR(t.m_ask - t.m_bid, 0.480, 1e-9);
    EXPECT_LT(t.m_bid, t.m_ask);
}

TEST(M1_Timeframe, EnumOrderMatchesBuilderSwitch) {
    EXPECT_EQ(static_cast<int>(Timeframe::TICK), 0);
    EXPECT_EQ(static_cast<int>(Timeframe::M1), 1);
    EXPECT_EQ(static_cast<int>(Timeframe::M5), 2);
    EXPECT_EQ(static_cast<int>(Timeframe::M15), 3);
    EXPECT_EQ(static_cast<int>(Timeframe::M30), 4);
    EXPECT_EQ(static_cast<int>(Timeframe::H1), 5);
    EXPECT_EQ(static_cast<int>(Timeframe::H4), 6);
}

TEST(M2_Aggregate, M1UsesMidAndTickCountAsVolume) {
    const auto day = make_date(2026, 7, 26);
    std::vector<Tick> ticks = {
        make_tick(day, make_tod(22, 1, 30, 255), 4091.771, 4092.251),
        make_tick(day, make_tod(22, 1, 30, 271), 4091.676, 4092.156),
        make_tick(day, make_tod(22, 1, 30, 626), 4092.261, 4092.741),
    };
    const auto c = aggregate_m1(ticks);
    EXPECT_TRUE(ohlc_ok(c));
    EXPECT_NEAR(c.m_open, mid_px(ticks.front()), 1e-12);
    EXPECT_NEAR(c.m_close, mid_px(ticks.back()), 1e-12);
    EXPECT_DOUBLE_EQ(c.m_tick_volume, 3.0);  // ISSUES.md #1 — tick count, not exchange volume
    EXPECT_GE(c.m_high, c.m_open);
    EXPECT_LE(c.m_low, c.m_open);
}

TEST(M2_Aggregate, HigherTfEnclosesLower) {
    const auto day = make_date(2026, 7, 26);
    std::vector<Candlestick> m1;
    for (int i = 0; i < 5; ++i) {
        const double base = 4090.0 + i;
        m1.push_back(make_candle(day, make_tod(22, i, 0), base, base + 2, base - 1, base + 0.5, 10));
    }
    const auto m5 = aggregate_from_lower(m1, 5);
    EXPECT_NEAR(m5.m_open, m1.front().m_open, 1e-12);
    EXPECT_NEAR(m5.m_close, m1.back().m_close, 1e-12);
    EXPECT_NEAR(m5.m_high, 4090.0 + 4 + 2, 1e-12);
    EXPECT_NEAR(m5.m_low, 4090.0 - 1, 1e-12);
    EXPECT_DOUBLE_EQ(m5.m_tick_volume, 50.0);
}

TEST(M2_Aggregate, HigherTfNeedsEnoughLowerBars) {
    const auto day = make_date(2026, 7, 26);
    std::vector<Candlestick> m1{make_candle(day, make_tod(22, 0, 0), 1, 2, 0.5, 1.5, 1)};
    const auto m5 = aggregate_from_lower(m1, 5);
    EXPECT_EQ(m5.m_tick_volume, 0.0);
}

TEST(M2_Builder, IngestMatchesOracleOnOneMinute) {
    const auto day = make_date(2026, 7, 26);
    std::vector<Tick> ticks = {
        make_tick(day, make_tod(22, 1, 30, 255), 4091.771, 4092.251),
        make_tick(day, make_tod(22, 1, 30, 271), 4091.676, 4092.156),
        make_tick(day, make_tod(22, 1, 30, 626), 4092.261, 4092.741),
    };
    const auto oracle = aggregate_m1(ticks);

    CandleStickBuilder b;
    b.ingest(ticks);
    b.flush();

    ASSERT_EQ(b.candles(Timeframe::M1).size(), 1u);
    const auto& c = b.candles(Timeframe::M1).front();
    EXPECT_TRUE(ohlc_ok(c));
    EXPECT_EQ(c.m_date, oracle.m_date);
    EXPECT_EQ(c.m_time, oracle.m_time);
    EXPECT_NEAR(c.m_open, oracle.m_open, 1e-12);
    EXPECT_NEAR(c.m_high, oracle.m_high, 1e-12);
    EXPECT_NEAR(c.m_low, oracle.m_low, 1e-12);
    EXPECT_NEAR(c.m_close, oracle.m_close, 1e-12);
    EXPECT_DOUBLE_EQ(c.m_tick_volume, 3.0);
    EXPECT_EQ(b.tick_count(), 3u);
}

TEST(M2_Builder, SplitsOnMinuteBoundaryNotWallClock) {
    const auto day = make_date(2026, 7, 26);
    std::vector<Tick> ticks = {
        make_tick(day, make_tod(22, 1, 30, 0), 4091.0, 4091.4),
        make_tick(day, make_tod(22, 2,  0, 0), 4092.0, 4092.4),
    };

    CandleStickBuilder b;
    b.ingest(ticks);
    b.flush();

    ASSERT_EQ(b.candles(Timeframe::M1).size(), 2u);
    EXPECT_EQ(b.candles(Timeframe::M1)[0].m_time, ticks[0].m_time);
    EXPECT_EQ(b.candles(Timeframe::M1)[1].m_time, ticks[1].m_time);
    EXPECT_DOUBLE_EQ(b.candles(Timeframe::M1)[0].m_tick_volume, 1.0);
    EXPECT_DOUBLE_EQ(b.candles(Timeframe::M1)[1].m_tick_volume, 1.0);

    // 22:01 and 22:02 sit in the same M5 bucket (22:00–22:04).
    ASSERT_EQ(b.candles(Timeframe::M5).size(), 1u);
    EXPECT_EQ(b.candles(Timeframe::M5).front().m_time, ticks[0].m_time);
    EXPECT_DOUBLE_EQ(b.candles(Timeframe::M5).front().m_tick_volume, 2.0);
}

TEST(M2_Builder, NewM5BucketAtFiveMinutes) {
    const auto day = make_date(2026, 7, 26);
    CandleStickBuilder b;
    b.ingest(make_tick(day, make_tod(22, 0, 0), 4090.0, 4090.4));
    b.ingest(make_tick(day, make_tod(22, 5, 0), 4091.0, 4091.4));
    b.flush();

    ASSERT_EQ(b.candles(Timeframe::M1).size(), 2u);
    ASSERT_EQ(b.candles(Timeframe::M5).size(), 2u);
}

TEST(M2_Builder, EmptyIsSafe) {
    CandleStickBuilder b;
    b.flush();
    EXPECT_TRUE(b.candles(Timeframe::M1).empty());
    EXPECT_FALSE(b.last_candle(Timeframe::M1).has_value());
    EXPECT_EQ(b.tick_count(), 0u);
}

TEST(M2_Builder, ReplayIsDeterministic) {
    const auto day = make_date(2026, 7, 26);
    std::vector<Tick> ticks = {
        make_tick(day, make_tod(22, 1, 30, 255), 4091.771, 4092.251),
        make_tick(day, make_tod(22, 1, 31, 0), 4092.344, 4092.824),
    };
    CandleStickBuilder a;
    CandleStickBuilder b;
    a.ingest(ticks);
    a.flush();
    b.ingest(ticks);
    b.flush();
    ASSERT_EQ(a.candles(Timeframe::M1).size(), b.candles(Timeframe::M1).size());
    EXPECT_EQ(a.candles(Timeframe::M1)[0].m_time, ticks.front().m_time);
    EXPECT_EQ(a.candles(Timeframe::M1)[0].m_time, b.candles(Timeframe::M1)[0].m_time);
    EXPECT_NEAR(a.candles(Timeframe::M1)[0].m_open, b.candles(Timeframe::M1)[0].m_open, 1e-12);
}
