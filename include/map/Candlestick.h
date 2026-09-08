#ifndef CANDLESTICK_H
#define CANDLESTICK_H

#include <queue> //std::queue
#include <vector> // std::vector
#include <optional> //std::optional
#include <chrono> //std::chrono::milliseconds
#include <stdexcept> // noexcept
#include <format> // std::format
#include <thread> //std::thread
#include <mutex> //std::mutex
#include <atomic> //std::atomic

#include "logging/Logging.h" // map::logging::Logger



namespace map::market_data {

    enum class Timeframe {
        TICK = 0, // only here for design and also live market updates, which exclude verbose info
        M1 = 1,
        M5 = 2,
        M15 = 3,
        M30 = 4,
        H1 = 5,
        H4 = 6,
    };

	//<DATE>	<TIME>	<OPEN>	<HIGH>	<LOW>	<CLOSE>	<TICKVOL>	<VOL>	<SPREAD>
    struct Candlestick {
        std::chrono::milliseconds   m_time;
        std::chrono::year_month_day m_date;
        double m_open;
        double m_high;
        double m_low;
        double m_price;
        double m_close;
        double m_tick_volume;
        double m_volume;
        double m_spread;

        Candlestick() : m_time({}), m_date({}), m_open(0.0) , m_high(0.0),
                        m_low(0.0), m_price(0.0), m_close(0.0), m_tick_volume(0.0),
                        m_volume(0.0), m_spread(0.0)
        {}

        Candlestick(std::chrono::milliseconds time,
            std::chrono::year_month_day date,
            double open, double high, double low,double price ,double close,
            double tick_volume, double volume, double spread)
            : m_time{ time }
            , m_date{ date }
            , m_open{ open }
            , m_high{ high }
            , m_price{ price }
            , m_low{ low }
            , m_close{ close }
            , m_tick_volume{ tick_volume }
            , m_volume{ volume }
            , m_spread{ spread }
        {}
    };

    // <DATE>	<TIME>	<BID>	<ASK>	<LAST>	<VOLUME>	<FLAGS>
    // TODO: This class I am planning on using to validate my tick class above. This is the actual tick class, that gets
    //       market events each time they change. The above is the tick every minute,5 minutes, etc
    struct Tick {
        std::chrono::milliseconds   m_time;
        std::chrono::year_month_day m_date;
        double m_bid; // times 100
        double m_ask;
        double m_last;
        double m_volume;
        size_t m_flags;

        Tick() {
            m_bid = 0;
            m_ask = 0;
            m_last = 0;
            m_volume = 0;
            m_flags = 0;
        }

        Tick(std::chrono::milliseconds time, std::chrono::year_month_day date,double bid, double ask, double last,
                double volume, size_t flags
        ) :
            m_time{time}, m_date{date}, m_bid{bid}, m_ask{ask}, m_last{last}, m_volume{volume} , m_flags{flags}
        {}
    };

    
    class CandleStickBuilder {
    public:
        CandleStickBuilder();
        [[nodisgard]] std::optional<Candlestick> get_candlestick(Timeframe timeframe = Timeframe::TICK, std::vector<Timeframe> tfs = {});
        ~CandleStickBuilder();
        Tick build_tick(std::string& path); // temporary - bad design
    private:
        std::jthread thr1;
        std::mutex access_control;
        std::atomic<bool> running;
        
        std::queue<Tick> ticks_;
        std::vector<Tick> processed_ticks;

        std::vector<Candlestick> minute_candlesticks;
        std::vector<Candlestick> minute_5_candlesticks;
        std::vector<Candlestick> minute_15_candlesticks;
        std::vector<Candlestick> minute_30_candlesticks;
        std::vector<Candlestick> hour_candlesticks;
        std::vector<Candlestick> hour_4_candlesticks;

        [[noreturn]]  void make_candlesticks(std::string path);
        void build_candlestick(Timeframe& tf);
        void build_candlestick(Timeframe&& tf);

        void build_m1();
        void build_from_lower(const std::vector<Candlestick>& lower, std::vector<Candlestick>& higher, size_t count);

        inline std::chrono::milliseconds make_time(int h, int m, int s, int ms) noexcept {
            return std::chrono::milliseconds{ (static_cast<long long>((h) * 3600 + m * 60 + s) * 1000 + ms) };
        }

        inline std::chrono::year_month_day make_date(int y, int m, int d) noexcept {
            using namespace std::chrono;
            return year{ y } / month{ static_cast<unsigned>(m) } / day{ static_cast<unsigned>(d) };
        }

        std::string to_human_time(std::chrono::milliseconds ms) noexcept {

            auto secs = duration_cast<std::chrono::seconds>(ms);
            std::chrono::hh_mm_ss time{ secs };

            auto millis = ms - duration_cast<std::chrono::milliseconds>(secs);

            return std::format("{:02}:{:02}:{:02}.{:03}",
                time.hours().count(),
                time.minutes().count(),
                time.seconds().count(),
                millis.count());
        }

    };
}










#endif // !CANDLESTICK_H