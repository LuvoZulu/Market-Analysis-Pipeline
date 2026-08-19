#include "map/Candlestick.h"

#include <string> //std::string, std::getline
#include <chrono> // std::chrono
#include <fstream> // std::ifstream
#include <filesystem> // std::filesystem::path
#include <ranges> // std::ranges::split
#include <string_view> // std::string_view
#include <charconv>  //std::from_chars
#include <vector> //std::vector
#include <print>  //std::println

namespace map::market_data {
    CandleStickBuilder::CandleStickBuilder()
    {
        // Initialize multithreading for candlestick processer
        thr1 = std::thread(make_candlesticks,this);
        running = true;
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

    Tick CandleStickBuilder::build_tick(std::string& path = "data\\gold.csv")
    {
        std::filesystem::path file_path = path;
        std::ifstream ticks_file(file_path);
        Tick tick{};

        if (!ticks_file.is_open()) {
            LOG_ERROR("Could not open file: {}", path);
            return tick;
        }

        std::string tick_event;
        while (std::getline(ticks_file, tick_event)) {
            if (tick_event.empty()) continue;

            auto split_str = tick_event
                | std::views::split('\t')
                | std::views::filter([](auto&& rng) { return !rng.empty(); })
                | std::views::transform([](auto&& rng) {
                return std::string(rng.begin(), rng.end());
                    });

            std::vector<std::string> fields(split_str.begin(), split_str.end());

            if (fields.size() < 5) {
                LOG_WARN("Skipping malformed line: {}", tick_event);
                continue;
            }

            {
                auto s_split = fields[0] | std::views::split('.')
                    | std::views::transform([](auto&& r) {
                    return std::string_view(r.begin(), r.end());
                        });
                std::vector<std::string_view> date_parts(s_split.begin(), s_split.end());

                if (date_parts.size() == 3) {
                    int y = 0, m = 0, d = 0;
                    std::from_chars(date_parts[0].data(), date_parts[0].data() + date_parts[0].size(), y);
                    std::from_chars(date_parts[1].data(), date_parts[1].data() + date_parts[1].size(), m);
                    std::from_chars(date_parts[2].data(), date_parts[2].data() + date_parts[2].size(), d);
                    tick.m_date = make_date(y, m, d);
                }
            }

            {
                auto s_split = fields[1] | std::views::split(':')
                    | std::views::transform([](auto&& r) {
                    return std::string_view(r.begin(), r.end());
                        });
                std::vector<std::string_view> time_parts(s_split.begin(), s_split.end());

                if (time_parts.size() >= 3) {
                    auto sec_ms = time_parts[2] | std::views::split('.')
                        | std::views::transform([](auto&& r) {
                        return std::string_view(r.begin(), r.end());
                            });
                    std::vector<std::string_view> ms_parts(sec_ms.begin(), sec_ms.end());

                    int h = 0, min = 0, s = 0, ms = 0;
                    std::from_chars(time_parts[0].data(), time_parts[0].data() + time_parts[0].size(), h);
                    std::from_chars(time_parts[1].data(), time_parts[1].data() + time_parts[1].size(), min);

                    if (ms_parts.size() >= 1)
                        std::from_chars(ms_parts[0].data(), ms_parts[0].data() + ms_parts[0].size(), s);
                    if (ms_parts.size() >= 2)
                        std::from_chars(ms_parts[1].data(), ms_parts[1].data() + ms_parts[1].size(), ms);

                    tick.m_time = make_time(h, min, s, ms);
                }
            }

            double ask = 0.0, bid = 0.0, flags = 0.0;
            std::from_chars(fields[2].data(), fields[2].data() + fields[2].size(), ask);
            std::from_chars(fields[3].data(), fields[3].data() + fields[3].size(), bid);
            std::from_chars(fields[4].data(), fields[4].data() + fields[4].size(), flags);

            tick.m_ask = ask;
            tick.m_bid = bid;
            tick.m_flags = flags;

            LOG_INFO("Tick: date={} time={} ask={}", tick.m_date, tick.m_time.count(), tick.m_ask);
        }

        return tick;
    }

	void CandleStickBuilder::make_candlesticks(std::string& path = "data\\gold.csv")
	{
        while (running) {
            
            {
                std::lock_guard<std::mutex> lock_time(access_control);
                auto now = std::chrono::system_clock::now();
                auto next = floor<std::chrono::minutes>(now) + std::chrono::minutes{ 1 };
                std::this_thread::sleep_until(next);
            }

            auto Ticks = build_tick(path);
            ticks_.push(Ticks);
        }
	}

    CandleStickBuilder::~CandleStickBuilder()
    {
        // join and stop all threads
    }
}