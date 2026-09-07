#include "technical/indicators/average_true_range.h"

namespace map::indicators
{
	double Atr::get_average(std::vector<map::market_data::Candlestick>&, std::chrono::year_month_day& duration)
	{
		return 0.0;
	}

	double Atr::get_average(std::vector<map::market_data::Candlestick>&, std::chrono::year_month_day& duration, std::chrono::milliseconds& to_time) 
	{
		return 0.0;
	}

	double Atr::get_average(std::vector<map::market_data::Candlestick>&, std::chrono::year_month_day& from, std::chrono::year_month_day& to)
	{
		return 0.0;
	}

	double Atr::get_average(std::vector<map::market_data::Candlestick>&, std::chrono::year_month_day& from_date, std::chrono::milliseconds& from_time, std::chrono::year_month_day& to, std::chrono::milliseconds& to_time) 
	{
		return 0.0;
	}
}
