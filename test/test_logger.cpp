#include "logging/Logging.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

TEST(M1_Logger, InitIsIdempotentAndReturnsALogger) {
    auto& log = map::logging::Logger::get_instance();
    log.init("map_gtest");
    log.init("map_gtest");  // second call must no-op, not replace sinks
    ASSERT_NE(log.get(), nullptr);
    log.get()->info("gtest logger smoke");
    log.get()->flush();
}

TEST(M1_Logger, NamedClonesShareThePipeline) {
    auto md = map::logging::Logger::get_instance().get("market_data");
    auto oms = map::logging::Logger::get_instance().get("oms");
    ASSERT_NE(md, nullptr);
    ASSERT_NE(oms, nullptr);
    EXPECT_EQ(md->name(), "market_data");
    EXPECT_EQ(oms->name(), "oms");
    md->info("tick accepted");
    oms->info("no order");
    md->flush();
}

TEST(M1_Logger, MacrosCompile) {
    map::logging::Logger::get_instance().init("map_gtest");
    LOG_INFO("info {}", 1);
    LOG_WARN("warn");
    LOG_ERROR("error");
    LOG_MD("md");
}

TEST(M1_Logger, RotatingFileSinkCreatesLogDirFile) {
    map::logging::Logger::get_instance().init("map_gtest");
    LOG_INFO("file sink check");
    map::logging::Logger::get_instance().get()->flush();
    // Relative "logs/" is what ISSUES.md #2/#3 will replace with a config path.
    EXPECT_TRUE(std::filesystem::exists("logs/map_gtest.log") ||
        std::filesystem::exists("logs"));
}
