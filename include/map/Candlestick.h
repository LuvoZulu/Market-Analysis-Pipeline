#ifndef CANDLESTICK_H
#define CANDLESTICK_H

#include <chrono>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace map::market_data {

    enum class Timeframe {
        TICK = 0,
        M1 = 1,
        M5 = 2,
        M15 = 3,
        M30 = 4,
        H1 = 5,
        H4 = 6,
    };

    [[nodiscard]] constexpr std::chrono::minutes timeframe_duration(Timeframe tf) noexcept {
        switch (tf) {
        case Timeframe::M1:  return std::chrono::minutes{ 1 };
        case Timeframe::M5:  return std::chrono::minutes{ 5 };
        case Timeframe::M15: return std::chrono::minutes{ 15 };
        case Timeframe::M30: return std::chrono::minutes{ 30 };
        case Timeframe::H1:  return std::chrono::minutes{ 60 };
        case Timeframe::H4:  return std::chrono::minutes{ 240 };
        case Timeframe::TICK:
        default:             return std::chrono::minutes{ 0 };
        }
    }

    // <DATE>	<TIME>	<OPEN>	<HIGH>	<LOW>	<CLOSE>	<TICKVOL>	<VOL>	<SPREAD>
    struct Candlestick {
        std::chrono::milliseconds   m_time{};
        std::chrono::year_month_day m_date{};
        double m_open{ 0.0 };
        double m_high{ 0.0 };
        double m_low{ 0.0 };
        double m_price{ 0.0 };
        double m_close{ 0.0 };
        double m_tick_volume{ 0.0 };
        double m_volume{ 0.0 };
        double m_spread{ 0.0 };

        Candlestick() = default;

        Candlestick(std::chrono::milliseconds time,
            std::chrono::year_month_day date,
            double open, double high, double low, double price, double close,
            double tick_volume, double volume, double spread)
            : m_time{ time }
            , m_date{ date }
            , m_open{ open }
            , m_high{ high }
            , m_low{ low }
            , m_price{ price }
            , m_close{ close }
            , m_tick_volume{ tick_volume }
            , m_volume{ volume }
            , m_spread{ spread }
        {}
    };

    // <DATE>	<TIME>	<BID>	<ASK>	<LAST>	<VOLUME>	<FLAGS>
    struct Tick {
        std::chrono::milliseconds   m_time{};
        std::chrono::year_month_day m_date{};
        double m_bid{ 0.0 };
        double m_ask{ 0.0 };
        double m_last{ 0.0 };
        double m_volume{ 0.0 };
        std::size_t m_flags{ 0 };

        Tick() = default;

        Tick(std::chrono::milliseconds time, std::chrono::year_month_day date,
            double bid, double ask, double last, double volume, std::size_t flags)
            : m_time{ time }
            , m_date{ date }
            , m_bid{ bid }
            , m_ask{ ask }
            , m_last{ last }
            , m_volume{ volume }
            , m_flags{ flags }
        {}

        [[nodiscard]] double mid() const noexcept {
            return (m_bid + m_ask) * 0.5;
        }

        // Prefer last trade; fall back to mid for OTC rows with empty LAST.
        [[nodiscard]] double trade_price() const noexcept {
            return m_last > 0.0 ? m_last : mid();
        }
    };

    [[nodiscard]] inline std::string to_human_time(std::chrono::milliseconds ms) {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(ms);
        const std::chrono::hh_mm_ss time{ secs };
        const auto millis = ms - std::chrono::duration_cast<std::chrono::milliseconds>(secs);
        return std::format("{:02}:{:02}:{:02}.{:03}",
            time.hours().count(),
            time.minutes().count(),
            time.seconds().count(),
            millis.count());
    }

    // Aggregates ticks into OHLC candles. CSV parsing lives in tick_csv.
    // No background thread: historical files are processed in-process from
    // tick timestamps. Live ingest / tools are later sprints (M4+).
    class CandleStickBuilder {
    public:
        CandleStickBuilder() = default;
        ~CandleStickBuilder() = default;

        CandleStickBuilder(const CandleStickBuilder&) = delete;
        CandleStickBuilder& operator=(const CandleStickBuilder&) = delete;
        CandleStickBuilder(CandleStickBuilder&&) noexcept = default;
        CandleStickBuilder& operator=(CandleStickBuilder&&) noexcept = default;

        // Parse with tick_csv, ingest in order, close any open candle.
        // Returns the number of ticks that parsed successfully.
        std::size_t load_from_csv(const std::filesystem::path& path);

        void ingest(const Tick& tick);
        void ingest(const std::vector<Tick>& ticks);

        // Close the in-progress M1 (and roll it into higher TFs).
        void flush();

        void clear();

        [[nodiscard]] const std::vector<Candlestick>& candles(Timeframe tf) const;
        [[nodiscard]] std::optional<Candlestick> last_candle(Timeframe tf) const;

        // Existing call site used M30. Kept so nothing silently changes.
        [[nodiscard]] const std::vector<Candlestick>& get_candlesticks() const {
            return candles(Timeframe::M30);
        }

        [[nodiscard]] const std::vector<Tick>& processed_ticks() const {
            return processed_ticks_;
        }

        [[nodiscard]] std::size_t tick_count() const {
            return processed_ticks_.size();
        }

    private:
        std::vector<Tick> processed_ticks_;

        std::vector<Candlestick> minute_candlesticks_;
        std::vector<Candlestick> minute_5_candlesticks_;
        std::vector<Candlestick> minute_15_candlesticks_;
        std::vector<Candlestick> minute_30_candlesticks_;
        std::vector<Candlestick> hour_candlesticks_;
        std::vector<Candlestick> hour_4_candlesticks_;

        std::optional<Candlestick> open_m1_;
        std::chrono::sys_time<std::chrono::milliseconds> open_m1_bucket_{};

        void close_open_m1();
        void roll_up(const Candlestick& m1);
        void merge_into(std::vector<Candlestick>& dest,
            const Candlestick& m1,
            std::chrono::minutes period);
    };

}  // namespace map::market_data

#endif  // !CANDLESTICK_H