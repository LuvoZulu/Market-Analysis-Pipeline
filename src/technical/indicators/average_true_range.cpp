#include "technical/indicators/average_true_range.h"

#include <atomic> // std::atomic
#include <algorithm> //std::max

namespace map::indicators
{
	double Atr::get_average(std::vector<map::market_data::Candlestick>& data, std::chrono::year_month_day& duration)
	{
		std::atomic<size_t> candlestick_num{ 0 };

		if (data.empty())
		{
			return 0.0;
		}

		auto curr_stick = data.cbegin();

		candlestick_num.fetch_add(1, std::memory_order_relaxed); // hard-coding incremenet (to be improved)

		double average = 0.0;

		map::market_data::Candlestick prev_stick;

		while ((curr_stick != data.cend()) && (curr_stick->m_date != duration)) {
			average += std::max({ abs(curr_stick->m_high - curr_stick->m_low),abs(curr_stick->m_high - prev_stick.m_close), abs(curr_stick->m_low - prev_stick.m_close) });
			candlestick_num.fetch_add(1, std::memory_order_relaxed); // hard-coding incremenet (to be improved)
		}

		return average / candlestick_num; // candlestick_num will never be 0
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
