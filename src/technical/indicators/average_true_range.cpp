#include "technical/indicators/average_true_range.h"

#include <atomic> // std::atomic
#include <algorithm> //std::max
#include <vector> // std::vector::const_iterator
#include <cmath> // std::abs

namespace map::indicators
{
    double Atr::get_average(std::vector<map::market_data::Candlestick>& data,
        std::chrono::year_month_day& target_date)
    {
        if (data.size() < 2) {
            return 0.0;
        }

        double sum_tr = 0.0;
        std::size_t count = 0;

        for (std::size_t i = 1; i < data.size(); ++i) {
            if (data[i].m_date != target_date) {
                continue;
            }

            const auto& curr = data[i];
            const auto& prev = data[i - 1];
            const double tr = std::max({
                std::abs(curr.m_high - curr.m_low),
                std::abs(curr.m_high - prev.m_close),
                std::abs(curr.m_low - prev.m_close)
                });
            sum_tr += tr;
            ++count;
        }

        return count > 0 ? sum_tr / static_cast<double>(count) : 0.0;
    }

	double Atr::get_average(std::vector<map::market_data::Candlestick>& data, std::chrono::year_month_day& duration, std::chrono::milliseconds& to_time) 
	{
        if (data.size() < 2) {
            return 0.0;
        }

        double sum_tr = 0.0;
        size_t count = 0;

        auto prev_stick = data.cbegin();
        auto curr_stick = std::next(prev_stick);

        while (curr_stick != data.cend()) {

            if (curr_stick->m_date > duration || (curr_stick->m_date == duration && curr_stick->m_time > to_time)) {
                break;
            }

            const double tr = std::max({std::abs(curr_stick->m_high - curr_stick->m_low),std::abs(curr_stick->m_high - prev_stick->m_close),std::abs(curr_stick->m_low - prev_stick->m_close)});

            sum_tr += tr;
            ++count;

            prev_stick = curr_stick;
            ++curr_stick;
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

        auto prev_stick = data.cbegin();
        auto curr_stick = std::next(prev_stick);

        while (curr_stick != data.cend()) {
            
            if (curr_stick->m_date < from) {
                prev_stick = curr_stick;
                ++curr_stick;
                continue;
            }else if (curr_stick->m_date > to) {
                break;
            }

            const double tr = std::max({std::abs(curr_stick->m_high - curr_stick->m_low),std::abs(curr_stick->m_high - prev_stick->m_close),std::abs(curr_stick->m_low - prev_stick->m_close)});

            sum_tr += tr;
            ++count;

            prev_stick = curr_stick;
            ++curr_stick;
        }

        return count > 0 ? sum_tr / count : 0.0;
	}

	double Atr::get_average(std::vector<map::market_data::Candlestick>& data, std::chrono::year_month_day& from_date, std::chrono::milliseconds& from_time, std::chrono::year_month_day& to_date, std::chrono::milliseconds& to_time) 
	{
        if (data.size() < 2) {
            return 0.0;
        }

        double sum_tr = 0.0;
        size_t count = 0;

        auto prev_stick = data.cbegin();
        auto curr_stick = std::next(prev_stick);

        while (curr_stick != data.cend()) {

            if ((curr_stick->m_date < from_date) || (curr_stick->m_time < to_time)) {
                prev_stick = curr_stick;
                ++curr_stick;
                continue;
            }
            else if ((curr_stick->m_date > to_date) && (curr_stick->m_time > to_time)) {
                break;
            }

            const double tr = std::max({ std::abs(curr_stick->m_high - curr_stick->m_low),std::abs(curr_stick->m_high - prev_stick->m_close),std::abs(curr_stick->m_low - prev_stick->m_close) });

            sum_tr += tr;
            ++count;

            prev_stick = curr_stick;
            ++curr_stick;
        }

        return count > 0 ? sum_tr / count : 0.0;
	}
}
