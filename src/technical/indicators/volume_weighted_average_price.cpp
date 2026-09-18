#include "technical/indicators/volume_weighted_average_price.h"

namespace map::indicators {
    namespace {

        double mid(const map::market_data::Tick& t) {
            if (t.m_last > 0.0) return t.m_last;
            return 0.5 * (t.m_bid + t.m_ask);
        }

        double typical(const map::market_data::Candlestick& c) {
            return (c.m_high + c.m_low + c.m_close) / 3.0;
        }

        double weight(const map::market_data::Candlestick& c) {
            return c.m_tick_volume > 0.0 ? c.m_tick_volume : 1.0;
        }

    }  // namespace

    double Vwap::get_average(const std::vector<map::market_data::Candlestick>& data) {
        double num = 0.0;
        double den = 0.0;
        for (const auto& c : data) {
            const double w = weight(c);
            num += typical(c) * w;
            den += w;
        }
        return den > 0.0 ? num / den : 0.0;
    }

    double Vwap::get_average(const std::vector<map::market_data::Candlestick>& data,
        const std::chrono::year_month_day& from,
        const std::chrono::year_month_day& to) {
        double num = 0.0;
        double den = 0.0;
        for (const auto& c : data) {
            if (c.m_date < from || c.m_date > to) continue;
            const double w = weight(c);
            num += typical(c) * w;
            den += w;
        }
        return den > 0.0 ? num / den : 0.0;
    }

    double Vwap::get_average_ticks(const std::vector<map::market_data::Tick>& ticks) {
        double num = 0.0;
        double den = 0.0;
        for (const auto& t : ticks) {
            num += mid(t);
            den += 1.0;
        }
        return den > 0.0 ? num / den : 0.0;
    }

}  // namespace map::indicators
