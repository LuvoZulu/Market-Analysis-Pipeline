// M6 — later sprint. Skip lives HERE, not in src/main.cpp.
#include "helpers.h"

#include <gtest/gtest.h>

using namespace map_test;

#if __has_include("technical/tools/trend.h")
#include "technical/tools/trend.h"
#define MAP_HAS_TREND 1
#else
#define MAP_HAS_TREND 0
#endif

#if __has_include("technical/tools/session.h")
#include "technical/tools/session.h"
#define MAP_HAS_SESSION 1
#else
#define MAP_HAS_SESSION 0
#endif

#if __has_include("technical/tools/candidate.h")
#include "technical/tools/candidate.h"
#define MAP_HAS_CANDIDATE 1
#else
#define MAP_HAS_CANDIDATE 0
#endif

TEST(M6_Session, UtcBuckets) {
    // gold.csv prints 22:01 — broker offset is ISSUES.md #4.
    // Contract: m_time is clock-of-day, interpreted as UTC until you store tz.
#if MAP_HAS_SESSION
    using map::technical::Session;
    using map::technical::detect_session;
    EXPECT_EQ(detect_session(make_tod(0, 0, 0)), Session::Tokyo);
    EXPECT_EQ(detect_session(make_tod(8, 0, 0)), Session::OverlapLN);
    EXPECT_EQ(detect_session(make_tod(13, 0, 0)), Session::OverlapNY);
    EXPECT_EQ(detect_session(make_tod(22, 1, 30)), Session::Off);
#else
    GTEST_SKIP() << "Add include/technical/tools/session.h — do not use system_clock::now()";
#endif
}

TEST(M6_Trend, HigherHighHigherLowIsUp) {
#if MAP_HAS_TREND
    GTEST_SKIP() << "wire detect_trend once momentum.h exists";
#else
    GTEST_SKIP() << "Add include/technical/tools/trend.h";
#endif
}

TEST(M6_Candidate, GeometryBuyStopBelowEntry) {
#if MAP_HAS_CANDIDATE
    map::technical::Candidate c{};
    c.side = map::technical::Side::Buy;
    c.entry = 4092;
    c.stop = 4088;
    c.target = 4100;
    EXPECT_LT(c.stop, c.entry);
    EXPECT_GT(c.target, c.entry);
    EXPECT_GE((c.target - c.entry) / (c.entry - c.stop), 1.0);
#else
    GTEST_SKIP() << "Add include/technical/tools/candidate.h";
#endif
}
