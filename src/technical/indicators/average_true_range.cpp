#include "technical/indicators/average_true_range.h"

#include <atomic> // std::atomic
#include <algorithm> //std::max
#include <vector> // std::vector::const_iterator
#include <cmath> // std::abs

namespace map::indicators
{
	double Atr::get_average(std::vector<map::market_data::Candlestick>& data, std::chrono::year_month_day& target_date)
	{
        if (data.size() < 2) {
            return 0.0;
        }

        double sum_tr = 0.0;
        size_t count = 0;

        auto prev = data.cbegin();
        auto curr = std::next(prev);

        while (curr != data.cend() && curr->m_date != target_date) {
            const double tr = std::max({std::abs(curr->m_high - curr->m_low),std::abs(curr->m_high - prev->m_close),std::abs(curr->m_low - prev->m_close)});

            sum_tr += tr;
            ++count;

            prev = curr;
            ++curr;
        }

        return count > 0 ? sum_tr / count : 0.0;
	}

	double Atr::get_average(std::vector<map::market_data::Candlestick>& data, std::chrono::year_month_day& duration, std::chrono::milliseconds& to_time) 
	{
        if (data.size() < 2) {
            return 0.0;
        }

        double sum_tr = 0.0;
        size_t count = 0;

        auto prev = data.cbegin();
        auto curr = std::next(prev);

        while (curr != data.cend()) {

            if (curr->m_date > duration ||
                (curr->m_date == duration && curr->m_time > to_time)) {
                break;
            }

            const double tr = std::max({std::abs(curr->m_high - curr->m_low),std::abs(curr->m_high - prev->m_close),std::abs(curr->m_low - prev->m_close)});

            sum_tr += tr;
            ++count;

            prev = curr;
            ++curr;
        }

        return count > 0 ? sum_tr / count : 0.0;
	}

	double Atr::get_average(std::vector<map::market_data::Candlestick>&data, std::chrono::year_month_day& from, std::chrono::year_month_day& to)
	{
        if (data.size() < 2) {
            return 0.0;
        }

        double sum_tr = 0.0;
        size_t count = 0;

        auto prev = data.cbegin();
        auto curr = std::next(prev);

        while (curr != data.cend()) {
            
            if (curr->m_date < from) {
                continue;
            }else if (curr->m_date > to) {
                break;
            }

            const double tr = std::max({std::abs(curr->m_high - curr->m_low),std::abs(curr->m_high - prev->m_close),std::abs(curr->m_low - prev->m_close)});

            sum_tr += tr;
            ++count;

            prev = curr;
            ++curr;
        }

        return count > 0 ? sum_tr / count : 0.0;
	}

	double Atr::get_average(std::vector<map::market_data::Candlestick>&, std::chrono::year_month_day& from_date, std::chrono::milliseconds& from_time, std::chrono::year_month_day& to, std::chrono::milliseconds& to_time) 
	{
		return 0.0;
	}
}
