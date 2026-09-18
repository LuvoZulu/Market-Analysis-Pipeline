#include "helpers.h"
#include "map/tick_csv.h"
#include "technical/indicators/average_true_range.h"
#include "technical/indicators/volume_weighted_average_price.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <map>

using namespace map_test;
using map::market_data::CandleStickBuilder;
using map::market_data::Timeframe;
using map::market_data::load_ticks_csv;

namespace {

std::filesystem::path gold_csv() {
    return std::filesystem::path("data/gold.csv");
}

// Floor time-of-day to a whole minute so we can compare builder buckets
// against the helpers.h oracle without depending on wall clock.
ms floor_minute(ms t) {
    const auto m = std::chrono::floor<std::chrono::minutes>(t);
    return std::chrono::duration_cast<ms>(m);
}

}  // namespace

// ---------------------------------------------------------------------------
// M7 contract tests — these run NOW. They use tick_csv + helpers + the builder.
// map/pipeline.h is a later sprint; that test is compiled only when the header
// exists. Do not skip this file in CMakeLists, and do not put SKIP in main.cpp.
// ---------------------------------------------------------------------------

TEST(M7_Pipeline, GoldCsvToM1ToVwapAtrIsDeterministic) {
    const auto path = gold_csv();
    if (!std::filesystem::exists(path)) {
        GTEST_SKIP() << "WORKING_DIRECTORY must be the repo root";
    }
    const auto ticks = load_ticks_csv(path);
    ASSERT_GE(ticks.size(), 10u);

    const auto m1 = aggregate_m1(ticks);
    EXPECT_TRUE(ohlc_ok(m1));
    EXPECT_DOUBLE_EQ(m1.m_tick_volume, static_cast<double>(ticks.size()));
    EXPECT_NEAR(m1.m_open, mid_px(ticks.front()), 1e-9);
    EXPECT_NEAR(m1.m_close, mid_px(ticks.back()), 1e-9);

    std::vector<Candlestick> bars{m1};
    map::indicators::Vwap vwap;
    const double v = vwap.get_average(bars);
    EXPECT_GE(v, m1.m_low);
    EXPECT_LE(v, m1.m_high);

    const auto ticks2 = load_ticks_csv(path);
    const auto m1b = aggregate_m1(ticks2);
    EXPECT_EQ(m1.m_tick_volume, m1b.m_tick_volume);
    EXPECT_NEAR(m1.m_open, m1b.m_open, 1e-12);
    EXPECT_NEAR(m1.m_close, m1b.m_close, 1e-12);
}

TEST(M7_Pipeline, DoesNotUseWallClock) {
    // ISSUES.md #4: aggregation is a function of tick timestamps, not
    // system_clock::now(). Locked for both the helper and the builder.
    const auto d = make_date(2026, 7, 26);
    std::vector<Tick> ticks{
        make_tick(d, make_tod(22, 1, 30, 255), 4091.771, 4092.251),
        make_tick(d, make_tod(22, 1, 31, 0), 4092.344, 4092.824),
    };
    const auto a = aggregate_m1(ticks);
    const auto b = aggregate_m1(ticks);
    EXPECT_EQ(a.m_date, d);
    EXPECT_EQ(a.m_time, ticks.front().m_time);
    EXPECT_EQ(a.m_date, b.m_date);
    EXPECT_EQ(a.m_time, b.m_time);

    CandleStickBuilder builder;
    builder.ingest(ticks);
    builder.flush();
    ASSERT_EQ(builder.candles(Timeframe::M1).size(), 1u);
    EXPECT_EQ(builder.candles(Timeframe::M1)[0].m_date, d);
    EXPECT_EQ(builder.candles(Timeframe::M1)[0].m_time, ticks.front().m_time);
}

TEST(M7_Pipeline, EmptyInputIsSafe) {
    std::vector<Tick> empty;
    const auto c = aggregate_m1(empty);
    EXPECT_EQ(c.m_tick_volume, 0.0);
    EXPECT_EQ(map::indicators::Vwap{}.get_average_ticks(empty), 0.0);

    CandleStickBuilder b;
    b.ingest(empty);
    b.flush();
    EXPECT_TRUE(b.candles(Timeframe::M1).empty());
}

TEST(M7_Pipeline, BuilderMatchesPerMinuteOracleOnGoldCsv) {
    const auto path = gold_csv();
    if (!std::filesystem::exists(path)) {
        GTEST_SKIP() << "WORKING_DIRECTORY must be the repo root";
    }
    const auto ticks = load_ticks_csv(path);
    ASSERT_GE(ticks.size(), 10u);

    CandleStickBuilder builder;
    const auto n = builder.load_from_csv(path);
    EXPECT_EQ(n, ticks.size());
    EXPECT_EQ(builder.tick_count(), ticks.size());

    std::map<std::pair<ymd, long long>, std::vector<Tick>> buckets;
    for (const auto& t : ticks) {
        buckets[{t.m_date, floor_minute(t.m_time).count()}].push_back(t);
    }

    const auto& m1 = builder.candles(Timeframe::M1);
    ASSERT_EQ(m1.size(), buckets.size());

    std::size_t i = 0;
    for (const auto& [key, group] : buckets) {
        (void)key;
        const auto oracle = aggregate_m1(group);
        EXPECT_TRUE(ohlc_ok(m1[i]));
        EXPECT_EQ(m1[i].m_date, oracle.m_date);
        EXPECT_EQ(m1[i].m_time, oracle.m_time);
        EXPECT_NEAR(m1[i].m_open, oracle.m_open, 1e-9);
        EXPECT_NEAR(m1[i].m_high, oracle.m_high, 1e-9);
        EXPECT_NEAR(m1[i].m_low, oracle.m_low, 1e-9);
        EXPECT_NEAR(m1[i].m_close, oracle.m_close, 1e-9);
        EXPECT_DOUBLE_EQ(m1[i].m_tick_volume, oracle.m_tick_volume);
        ++i;
    }

    map::indicators::Vwap vwap;
    const double v = vwap.get_average(m1);
    for (const auto& c : m1) {
        EXPECT_TRUE(ohlc_ok(c));
    }
    EXPECT_GT(v, 0.0);

    CandleStickBuilder replay;
    replay.load_from_csv(path);
    ASSERT_EQ(replay.candles(Timeframe::M1).size(), m1.size());
    for (std::size_t k = 0; k < m1.size(); ++k) {
        EXPECT_NEAR(replay.candles(Timeframe::M1)[k].m_open, m1[k].m_open, 1e-12);
        EXPECT_NEAR(replay.candles(Timeframe::M1)[k].m_close, m1[k].m_close, 1e-12);
        EXPECT_EQ(replay.candles(Timeframe::M1)[k].m_tick_volume, m1[k].m_tick_volume);
    }
}

#if __has_include("map/pipeline.h")
#include "map/pipeline.h"
TEST(M7_Pipeline, ProductionRunPipeline) {
    const auto path = gold_csv();
    if (!std::filesystem::exists(path)) GTEST_SKIP();
    const auto ticks = load_ticks_csv(path);
    const auto a = map::run_pipeline(ticks, Timeframe::M1);
    const auto b = map::run_pipeline(ticks, Timeframe::M1);
    ASSERT_EQ(a.candles.size(), b.candles.size());
    ASSERT_FALSE(a.candles.empty());
    for (const auto& c : a.candles) EXPECT_TRUE(ohlc_ok(c));
}
#else
TEST(M7_Pipeline, ProductionRunPipeline) {
    GTEST_SKIP() << "Add include/map/pipeline.h — M7 production runner, later sprint";
}
#endif
