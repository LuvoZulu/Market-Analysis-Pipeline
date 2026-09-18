#ifndef VWAP_H
#define VWAP_H

#include "map/Candlestick.h"

#include <chrono>
#include <vector>

namespace map::indicators {

    // Typical-price VWAP using tick_volume as the weight (OTC has no real volume).
    class Vwap {
    public:
        Vwap() = default;

        [[nodiscard]] double get_average(const std::vector<map::market_data::Candlestick>& data);

        [[nodiscard]] double get_average(const std::vector<map::market_data::Candlestick>& data,
            const std::chrono::year_month_day& from,
            const std::chrono::year_month_day& to);

        [[nodiscard]] double get_average_ticks(const std::vector<map::market_data::Tick>& ticks);

        ~Vwap() = default;
    };

}  // namespace map::indicators

#endif
