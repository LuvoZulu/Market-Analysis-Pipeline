#include <map/Candlestick.h>
#include <iostream>

int main() {
    map::logging::Logger::get_instance().init("Market_Analysis_Pipeline");

    LOG_INFO("Application started");

    try {
        map::market_data::CandleStickBuilder builder;
        std::string c = "..\\..\\data\\gold.csv";

        builder.build_tick(c);

        LOG_INFO("build_tick finished successfully");
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