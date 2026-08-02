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
}










#endif // !TICK_H