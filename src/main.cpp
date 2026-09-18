#include <map/Candlestick.h>
#include <map/tick_csv.h>

#include <iostream>
#include <filesystem>

int main() {
    map::logging::Logger::get_instance().init("Market_Analysis_Pipeline");

    LOG_INFO("Application started");

    try {
        map::market_data::CandleStickBuilder builder;
        const std::filesystem::path csv = std::filesystem::path{ "data" } / "gold.csv";

        const std::size_t n = builder.load_from_csv(csv);
        LOG_INFO("Loaded {} ticks from {}", n, csv.string());

        if (auto m1 = builder.last_candle(map::market_data::Timeframe::M1)) {
            LOG_INFO("Last M1 O={} H={} L={} C={} tv={}",
                m1->m_open, m1->m_high, m1->m_low, m1->m_close, m1->m_tick_volume);
        }

        LOG_INFO("M1={} M5={} M15={} M30={} H1={} H4={}",
            builder.candles(map::market_data::Timeframe::M1).size(),
            builder.candles(map::market_data::Timeframe::M5).size(),
            builder.candles(map::market_data::Timeframe::M15).size(),
            builder.candles(map::market_data::Timeframe::M30).size(),
            builder.candles(map::market_data::Timeframe::H1).size(),
            builder.candles(map::market_data::Timeframe::H4).size());
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception: {}", e.what());
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    catch (...) {
        LOG_ERROR("Unknown exception occurred");
        std::cerr << "Unknown exception occurred" << std::endl;
    }

    LOG_INFO("Application stopping");
    return 0;
}