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
#include <limits> //std::numerics

namespace map::market_data {
    CandleStickBuilder::CandleStickBuilder()
    {
        // Initialize multithreading for candlestick processer
        std::string path = "..\\data\\gold.csv";
        thr1 = std::jthread(make_candlesticks,std::move(path),this);
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

    Tick CandleStickBuilder::build_tick(std::string& path)
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

    void CandleStickBuilder::make_candlesticks(std::string& path) 
    {
        using namespace std::chrono;

        auto next_minute = floor<minutes>(system_clock::now()) + minutes{ 1 };
        int minute_counter = 0;

        while (running.load(std::memory_order_relaxed)) {

            while (system_clock::now() < next_minute && running.load(std::memory_order_relaxed)) {
                auto tick = build_tick(path);

                {
                    std::lock_guard lock(access_control);
                    ticks_.emplace(std::move(tick));
                }

                std::this_thread::sleep_for(milliseconds{ 5 });
            }

            if (!running.load(std::memory_order_relaxed))
                break;

            ++minute_counter;

            build_candlestick(Timeframe::M1);

            if (minute_counter % 5 == 0)   build_candlestick(Timeframe::M5);
            if (minute_counter % 15 == 0)  build_candlestick(Timeframe::M15);
            if (minute_counter % 30 == 0)  build_candlestick(Timeframe::M30);
            if (minute_counter % 60 == 0)  build_candlestick(Timeframe::H1);
            if (minute_counter % 240 == 0) build_candlestick(Timeframe::H4);

            next_minute += minutes{ 1 };
        }
    }

    void CandleStickBuilder::build_candlestick(Timeframe& tf)
    {
            switch (tf) {
            case Timeframe::M1:
                build_m1();
                break;
            case Timeframe::M5:
                build_from_lower(minute_candlesticks, minute_5_candlesticks, 5);
                break;
            case Timeframe::M15:
                build_from_lower(minute_5_candlesticks, minute_15_candlesticks, 3);
                break;
            case Timeframe::M30:
                build_from_lower(minute_15_candlesticks, minute_30_candlesticks, 2);
                break;
            case Timeframe::H1:
                build_from_lower(minute_30_candlesticks, hour_candlesticks, 2);
                break;
            case Timeframe::H4:
                build_from_lower(hour_candlesticks, hour_4_candlesticks, 4);
                break;
            default:
                break;
            }
    }

    void CandleStickBuilder::build_candlestick(Timeframe&& tf)
    {
        Timeframe current_view = std::move(tf);

        switch (current_view) {
            case Timeframe::M1:
                build_m1();
                break;
            case Timeframe::M5:
                build_from_lower(minute_candlesticks, minute_5_candlesticks, 5);
                break;
            case Timeframe::M15:
                build_from_lower(minute_5_candlesticks, minute_15_candlesticks, 3);
                break;
            case Timeframe::M30:
                build_from_lower(minute_15_candlesticks, minute_30_candlesticks, 2);
                break;
            case Timeframe::H1:
                build_from_lower(minute_30_candlesticks, hour_candlesticks, 2);
                break;
            case Timeframe::H4:
                build_from_lower(hour_candlesticks, hour_4_candlesticks, 4);
                break;
            default:
                break;
        }
    }

    void CandleStickBuilder::build_m1()
    {
        if (ticks_.empty())
            return;

        double high = -std::numeric_limits<double>::infinity();
        double low = std::numeric_limits<double>::infinity();
        double volume = 0.0;

        Tick open_tick = ticks_.front();
        double open_price = (open_tick.m_bid + open_tick.m_ask) * 0.5;

        Tick last_tick = open_tick;

        while (!ticks_.empty()) {
            Tick tick = ticks_.front();
            ticks_.pop();

            double price = (tick.m_bid + tick.m_ask) * 0.5;

            high = std::max(high, price);
            low = std::min(low, price);
            volume += tick.m_volume;

            last_tick = tick;

            
            processed_ticks.emplace_back(tick);
        }

        double close_price = (last_tick.m_bid + last_tick.m_ask) * 0.5;
        double spread = last_tick.m_ask - last_tick.m_bid;

        minute_candlesticks.emplace_back(open_tick.m_time,open_tick.m_date,open_price,high,low,close_price,volume,volume,spread);
    }

    void CandleStickBuilder::build_from_lower(const std::vector<Candlestick>& lower,std::vector<Candlestick>& higher,size_t count)
    {
        if (lower.size() < count)
            return;
        size_t start = lower.size() - count;

        const Candlestick& open_stick = lower[start];
        const Candlestick& close_stick = lower.back();

        double high = -std::numeric_limits<double>::infinity();
        double low = std::numeric_limits<double>::infinity();
        double volume = 0.0;

        for (size_t i = start; i < lower.size(); ++i) {
            high = std::max(high, lower[i].m_high);
            low = std::min(low, lower[i].m_low);
            volume += lower[i].m_volume;
        }

        double spread = close_stick.m_spread;

        higher.emplace_back(open_stick.m_time,open_stick.m_date,open_stick.m_price,high,low,close_stick.m_close,volume,volume,spread);
    }

    CandleStickBuilder::~CandleStickBuilder()
    {
        // join and stop all threads
        thr1.request_stop();
    }
}