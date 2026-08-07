#include "map/Candlestick.h"

#include <string> //std::string, std::getline
#include <chrono> // std::chrono
#include <fstream> // std::ifstream
#include <filesystem> // std::filesystem::path
#include <ranges> // std::ranges::split
#include <string_view> // std::string_view

namespace map::market_data {
    CandleStickBuilder::CandleStickBuilder()
    {
        // Initialize multithreading for candlestick processer
    }
    std::optional<Candlestick> CandleStickBuilder::get_candlestick(Timeframe timeframe,std::vector<Timeframe> tfs)
    {
        if (tfs.empty() && timeframe == Timeframe::TICK) {
            //if (ticks_.empty())
                return std::nullopt;
            //return std::optional{ticks_.front()};
        }

        switch (timeframe) {
        case Timeframe::M1:
            return minute_candlesticks.empty()
                ? std::nullopt
                : std::optional{ minute_candlesticks.back() };

        case Timeframe::M5:
            return minute_5_candlesticks.empty()
                ? std::nullopt
                : std::optional{ minute_5_candlesticks.back() };

        case Timeframe::M15:
            return minute_15_candlesticks.empty()
                ? std::nullopt
                : std::optional{ minute_15_candlesticks.back() };

        case Timeframe::M30:
            return minute_30_candlesticks.empty()
                ? std::nullopt
                : std::optional{ minute_30_candlesticks.back() };

        case Timeframe::H1:
            return hour_candlesticks.empty()
                ? std::nullopt
                : std::optional{ hour_candlesticks.back() };

        case Timeframe::H4:
            return hour_4_candlesticks.empty()
                ? std::nullopt
                : std::optional{ hour_4_candlesticks.back() };

        case Timeframe::TICK:
        default:
            //if (ticks_.empty())
                return std::nullopt;
            // return std::optional{ ticks_.front() }; // to add valid conversion from Candlestick to Tick and/or visa vers
        }
    }

    CandleStickBuilder::~CandleStickBuilder()
    {
        // join and stop all threads
    }

	Tick CandleStickBuilder::build_tick(std::string& path)
	{
        std::filesystem::path file_path = path;

        std::ifstream ticks_file(file_path);

        if (ticks_file.is_open()) {
            std::string tick_event;

            while (std::getline(ticks_file,tick_event)) {

                auto split_str = tick_event | std::views::split(' ') | std::views::transform([](auto&& rngs)) {
                    return std::string(rngs.begin(), rngs.end());
                }) | std::views::filter([](std::string_view sv) {
                    return !sv.empty();
                    });

                auto idx = 0uz;

                for (std::string_view sv : split_str) {
                    switch (idx) {
                    case 0:
                        break;
                    case 1:
                        break;
                    case 2:
                        break;
                    case 3:
                        break;
                    }
                }
                Tick tick;

            }
        }
		return Tick();
	}

	void CandleStickBuilder::make_candlesticks(std::string& path)
	{

	}
}