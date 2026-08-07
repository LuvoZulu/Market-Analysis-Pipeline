#include "Candlestick.h"


namespace map::market_data {

    std::optional<Candlestick> CandleStickBuilder::get_candlestick(Timeframe timeframe = Timeframe::TICK,std::vector<Timeframe> tfs = {})
    {
        if (tfs.empty() && timeframe == Timeframe::TICK) {
            if (ticks_.empty())
                return std::nullopt;
            return ticks_.front();
        }

        switch (timeframe) {
        case Timeframe::M1:
            return minute_candlesticks.empty()
                ? std::nullopt
                : minute_candlesticks.back();

        case Timeframe::M5:
            return minute_5_candlesticks.empty()
                ? std::nullopt
                : minute_5_candlesticks.back();

        case Timeframe::M15:
            return minute_15_candlesticks.empty()
                ? std::nullopt
                : minute_15_candlesticks.back();

        case Timeframe::M30:
            return minute_30_candlesticks.empty()
                ? std::nullopt
                : minute_30_candlesticks.back();

        case Timeframe::H1:
            return hour_candlesticks.empty()
                ? std::nullopt
                : hour_candlesticks.back();

        case Timeframe::H4:
            return hour_4_candlesticks.empty()
                ? std::nullopt
                : hour_4_candlesticks.back();

        case Timeframe::TICK:
        default:
            if (ticks_.empty())
                return std::nullopt;
            return ticks_.front();
        }
    }

	Tick CandleStickBuilder::build_tick(std::string& path)
	{
		return Tick();
	}

	void CandleStickBuilder::make_candlesticks(std::string& path)
	{

	}
}