#include "helpers.h"
#include "map/tick_csv.h"

#include <gtest/gtest.h>

#include <filesystem>

using map::market_data::load_ticks_csv;
using map::market_data::parse_tick_line;
using map::market_data::TickParseError;

TEST(M1_TickCsv, ParsesMt5HeaderOrderAndKeepsEmptyColumns) {
    // gold.csv has empty LAST and VOLUME. Dropping empty fields shifts FLAGS
    // into the LAST column — that is the current build_tick bug.
    const char* line = "2026.07.26\t22:01:30.255\t4091.771\t4092.251\t\t\t6";
    TickParseError err;
    const auto t = parse_tick_line(line, &err);
    ASSERT_TRUE(t.has_value()) << err.why;
    EXPECT_EQ(t->m_date, map_test::make_date(2026, 7, 26));
    EXPECT_EQ(t->m_time, map_test::make_tod(22, 1, 30, 255));
    EXPECT_NEAR(t->m_bid, 4091.771, 1e-9);
    EXPECT_NEAR(t->m_ask, 4092.251, 1e-9);
    EXPECT_EQ(t->m_last, 0.0);
    EXPECT_EQ(t->m_volume, 0.0);
    EXPECT_EQ(t->m_flags, 6u);
    EXPECT_LT(t->m_bid, t->m_ask);
}

TEST(M1_TickCsv, RejectsCrossedBookAndNonPositive) {
    EXPECT_FALSE(parse_tick_line("2026.07.26\t22:01:30.255\t4092.251\t4091.771\t\t\t6"));
    EXPECT_FALSE(parse_tick_line("2026.07.26\t22:01:30.255\t0\t0\t\t\t6"));
    EXPECT_FALSE(parse_tick_line("not-a-tick"));
}

TEST(M1_TickCsv, SkipsHeader) {
    EXPECT_FALSE(parse_tick_line("<DATE>\t<TIME>\t<BID>\t<ASK>\t<LAST>\t<VOLUME>\t<FLAGS>"));
}

TEST(M1_TickCsv, LoadsGoldFixture) {
    const auto path = std::filesystem::path("data/gold.csv");
    if (!std::filesystem::exists(path)) {
        GTEST_SKIP() << "run ctest with WORKING_DIRECTORY = repo root (data/gold.csv)";
    }
    std::vector<TickParseError> errors;
    const auto ticks = load_ticks_csv(path, &errors);
    EXPECT_TRUE(errors.empty());
    ASSERT_GE(ticks.size(), 20u);
    EXPECT_EQ(ticks.front().m_date, map_test::make_date(2026, 7, 26));
    EXPECT_EQ(ticks.front().m_time, map_test::make_tod(22, 1, 30, 255));
    EXPECT_NEAR(ticks.front().m_bid, 4091.771, 1e-9);
    EXPECT_NEAR(ticks.front().m_ask, 4092.251, 1e-9);
    for (const auto& t : ticks) {
        EXPECT_GT(t.m_bid, 0.0);
        EXPECT_GT(t.m_ask, 0.0);
        EXPECT_LE(t.m_bid, t.m_ask);
        EXPECT_EQ(t.m_volume, 0.0) << "OTC gold.csv has empty VOLUME — do not invent it";
    }
}

TEST(M1_TickCsv, GoldTimestampsAreMonotonic) {
    const auto path = std::filesystem::path("data/gold.csv");
    if (!std::filesystem::exists(path)) GTEST_SKIP();
    const auto ticks = load_ticks_csv(path);
    ASSERT_GE(ticks.size(), 2u);
    for (std::size_t i = 1; i < ticks.size(); ++i) {
        EXPECT_LE(ticks[i - 1].m_time, ticks[i].m_time);
    }
}
