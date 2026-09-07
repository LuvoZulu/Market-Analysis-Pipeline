#ifndef ATR_H
#define ATR_H

#include "logging/Logging.h"
#include "map/Candlestick.h"


#include <vector> // std::vector
#include <chrono> //std::chrono

namespace map::indicators
{

	class Atr {
	public:
		Atr() = default;
		[[nodiscard]] double get_average(std::vector<map::market_data::Candlestick>&, std::chrono::year_month_day& duration);
		[[nodiscard]] double get_average(std::vector<map::market_data::Candlestick>&, std::chrono::year_month_day& duration, std::chrono::milliseconds& to_time);
		[[nodiscard]] double get_average(std::vector<map::market_data::Candlestick>&, std::chrono::year_month_day& from, std::chrono::year_month_day& to);
		[[nodiscard]] double get_average(std::vector<map::market_data::Candlestick>&, std::chrono::year_month_day& from_date, std::chrono::milliseconds& from_time, std::chrono::year_month_day& to, std::chrono::milliseconds& to_time);
		~Atr() = default;
	};

}

#endif // !ATR_H