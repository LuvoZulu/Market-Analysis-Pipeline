#include "map/Candlestick.h"
#include "map/tick_csv.h"

#include "logging/Logging.h"

#include <algorithm>

namespace map::market_data {
    namespace {

        using Stamp = std::chrono::sys_time<std::chrono::milliseconds>;

        Stamp stamp_of(std::chrono::year_month_day date,
            std::chrono::milliseconds tod) {
            return std::chrono::sys_days{ date } + tod;
        }

        Stamp stamp_of(const Tick& t) {
            return stamp_of(t.m_date, t.m_time);
        }

        Stamp stamp_of(const Candlestick& c) {
            return stamp_of(c.m_date, c.m_time);
        }

        Stamp floor_to(Stamp t, std::chrono::minutes period) {
            if (period.count() <= 0) {
                return t;
            }
            const auto mins = std::chrono::floor<std::chrono::minutes>(t);
            const auto n = mins.time_since_epoch() / period;
            return Stamp{ n * period };
        }

        // Contract (helpers.h / ISSUES.md #1):
        //   price = last if present else mid(bid, ask)
        //   m_tick_volume = tick count
        //   m_time/m_date of an M1 = first tick in the minute (not wall clock, not floored)
        Candlestick candle_from_tick(const Tick& tick) {
            const double px = tick.trade_price();
            return Candlestick{
                tick.m_time,
                tick.m_date,
                px, px, px, px, px,
                1.0,
                1.0,
                tick.m_ask - tick.m_bid
            };
        }

        void apply_tick(Candlestick& c, const Tick& tick) {
            const double px = tick.trade_price();
            c.m_high = std::max(c.m_high, px);
            c.m_low = std::min(c.m_low, px);
            c.m_close = px;
            c.m_price = px;
            c.m_tick_volume += 1.0;
            c.m_volume += 1.0;
            c.m_spread = tick.m_ask - tick.m_bid;
        }

        void merge_candle(Candlestick& dest, const Candlestick& src) {
            dest.m_high = std::max(dest.m_high, src.m_high);
            dest.m_low = std::min(dest.m_low, src.m_low);
            dest.m_close = src.m_close;
            dest.m_price = src.m_close;
            dest.m_tick_volume += src.m_tick_volume;
            dest.m_volume += src.m_volume;
            dest.m_spread = src.m_spread;
        }

    }  // namespace

    std::size_t CandleStickBuilder::load_from_csv(const std::filesystem::path& path) {
        std::vector<TickParseError> errors;
        auto ticks = load_ticks_csv(path, &errors);

        for (const auto& e : errors) {
            LOG_WARN("tick parse L{} ({}): {}", e.line, e.why, e.raw);
        }

        if (ticks.empty() && !std::filesystem::exists(path)) {
            LOG_ERROR("Could not open file: {}", path.string());
            return 0;
        }

        ingest(ticks);
        flush();
        return ticks.size();
    }

    void CandleStickBuilder::ingest(const std::vector<Tick>& ticks) {
        for (const auto& t : ticks) {
            ingest(t);
        }
    }

    void CandleStickBuilder::ingest(const Tick& tick) {
        if (!tick.m_date.ok()) {
            LOG_WARN("Skipping tick with invalid date");
            return;
        }

        const Stamp ts = stamp_of(tick);
        const Stamp bucket = floor_to(ts, std::chrono::minutes{ 1 });

        if (open_m1_ && bucket < open_m1_bucket_) {
            LOG_WARN("Skipping out-of-order tick at {}", to_human_time(tick.m_time));
            return;
        }
        if (!open_m1_ && !minute_candlesticks_.empty()) {
            const Stamp last_bucket =
                floor_to(stamp_of(minute_candlesticks_.back()), std::chrono::minutes{ 1 });
            if (bucket < last_bucket) {
                LOG_WARN("Skipping out-of-order tick at {}", to_human_time(tick.m_time));
                return;
            }
        }

        if (open_m1_ && bucket != open_m1_bucket_) {
            close_open_m1();
        }

        if (!open_m1_) {
            open_m1_ = candle_from_tick(tick);
            open_m1_bucket_ = bucket;
        }
        else {
            apply_tick(*open_m1_, tick);
        }

        processed_ticks_.push_back(tick);
    }

    void CandleStickBuilder::flush() {
        close_open_m1();
    }

    void CandleStickBuilder::clear() {
        processed_ticks_.clear();
        minute_candlesticks_.clear();
        minute_5_candlesticks_.clear();
        minute_15_candlesticks_.clear();
        minute_30_candlesticks_.clear();
        hour_candlesticks_.clear();
        hour_4_candlesticks_.clear();
        open_m1_.reset();
        open_m1_bucket_ = {};
    }

    const std::vector<Candlestick>& CandleStickBuilder::candles(Timeframe tf) const {
        switch (tf) {
        case Timeframe::M1:  return minute_candlesticks_;
        case Timeframe::M5:  return minute_5_candlesticks_;
        case Timeframe::M15: return minute_15_candlesticks_;
        case Timeframe::M30: return minute_30_candlesticks_;
        case Timeframe::H1:  return hour_candlesticks_;
        case Timeframe::H4:  return hour_4_candlesticks_;
        case Timeframe::TICK:
        default: {
            static const std::vector<Candlestick> empty;
            return empty;
        }
        }
    }

    std::optional<Candlestick> CandleStickBuilder::last_candle(Timeframe tf) const {
        const auto& v = candles(tf);
        if (v.empty()) {
            return std::nullopt;
        }
        return v.back();
    }

    void CandleStickBuilder::close_open_m1() {
        if (!open_m1_) {
            return;
        }
        minute_candlesticks_.push_back(*open_m1_);
        roll_up(*open_m1_);
        open_m1_.reset();
    }

    void CandleStickBuilder::roll_up(const Candlestick& m1) {
        merge_into(minute_5_candlesticks_, m1, std::chrono::minutes{ 5 });
        merge_into(minute_15_candlesticks_, m1, std::chrono::minutes{ 15 });
        merge_into(minute_30_candlesticks_, m1, std::chrono::minutes{ 30 });
        merge_into(hour_candlesticks_, m1, std::chrono::minutes{ 60 });
        merge_into(hour_4_candlesticks_, m1, std::chrono::minutes{ 240 });
    }

    void CandleStickBuilder::merge_into(std::vector<Candlestick>& dest,
        const Candlestick& m1,
        std::chrono::minutes period) 
    {
        const Stamp bucket = floor_to(stamp_of(m1), period);

        if (dest.empty() || floor_to(stamp_of(dest.back()), period) != bucket) {
            dest.push_back(m1);
            return;
        }

        merge_candle(dest.back(), m1);
    }

}  // namespace map::market_data