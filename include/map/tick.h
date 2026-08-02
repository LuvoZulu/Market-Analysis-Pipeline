#ifndef TICK_H
#define TICK_H

#include <chrono> // std::chrono

#include "logging/Logging.h" // map::logging::Logger



namespace map::market_data{

	//<DATE>	<TIME>	<OPEN>	<HIGH>	<LOW>	<CLOSE>	<TICKVOL>	<VOL>	<SPREAD>
    struct Tick {
        std::chrono::milliseconds   m_time;
        std::chrono::year_month_day m_date;
        double m_open;
        double m_high;
        double m_low;
        double m_close;
        double m_tick_volume;
        double m_volume;
        double m_spread;

        Tick(std::chrono::milliseconds time,
            std::chrono::year_month_day date,
            double open, double high, double low, double close,
            double tick_volume, double volume, double spread)
            : m_time{ time }
            , m_date{ date }
            , m_open{ open }
            , m_high{ high }
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
    struct Tick_v {
        std::chrono::milliseconds   m_time;
        std::chrono::year_month_day m_date;
        double m_bid;
        double m_ask;
        double m_last;
        double m_volume;
        size_t flags;

        Tick_v(std::chrono::milliseconds time, std::chrono::year_month_day date,double bid, double ask, double last,
                double volume, size_t flags
        ) :
            m_time{time}, m_date{date}, m_bid{bid}, m_ask{ask}, m_last{last}, m_volume{volume} , m_flags{flags}
        {}
    };

    class TickBuilder {
    public:
        TickBuilder(){}
        ~TickBuilder(){}
    };
}










#endif // !TICK_H