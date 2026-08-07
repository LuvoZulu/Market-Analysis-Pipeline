#include <map/Candlestick.h>

int main() {
    
    map::logging::Logger::get_instance().init("Market_Analysis_Pipeline");
    
    LOG_INFO("Application started");
    
    map::market_data::CandleStickBuilder buidler;
    std::string c = "C:\\Users\\Kaos\\Documents\\2026\\Programming\\Quant\\Market-Analysis-Pipeline\\data\\gold.csv";

    buidler.build_tick(c);

    LOG_INFO("Application stopping");

    return 0;
}