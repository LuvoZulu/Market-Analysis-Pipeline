#include "map/tick_csv.h"

#include <charconv>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>

namespace map::market_data {
    namespace {

        std::chrono::year_month_day parse_date(std::string_view s) {
            int y = 0, m = 0, d = 0;
            const auto p1 = s.find('.');
            const auto p2 = s.find('.', p1 + 1);
            if (p1 == std::string_view::npos || p2 == std::string_view::npos) return {};
            std::from_chars(s.data(), s.data() + p1, y);
            std::from_chars(s.data() + p1 + 1, s.data() + p2, m);
            std::from_chars(s.data() + p2 + 1, s.data() + s.size(), d);
            return std::chrono::year{ y } / std::chrono::month{ static_cast<unsigned>(m) } /
                std::chrono::day{ static_cast<unsigned>(d) };
        }

        std::chrono::milliseconds parse_tod(std::string_view s) {
            int h = 0, min = 0, sec = 0, ms = 0;
            const auto c1 = s.find(':');
            const auto c2 = s.find(':', c1 + 1);
            if (c1 == std::string_view::npos || c2 == std::string_view::npos) return {};
            std::from_chars(s.data(), s.data() + c1, h);
            std::from_chars(s.data() + c1 + 1, s.data() + c2, min);
            const auto dot = s.find('.', c2 + 1);
            if (dot == std::string_view::npos) {
                std::from_chars(s.data() + c2 + 1, s.data() + s.size(), sec);
            }
            else {
                std::from_chars(s.data() + c2 + 1, s.data() + dot, sec);
                std::from_chars(s.data() + dot + 1, s.data() + s.size(), ms);
            }
            return std::chrono::milliseconds{
                (static_cast<long long>(h) * 3600 + min * 60 + sec) * 1000 + ms };
        }

        double parse_d(std::string_view s) {
            if (s.empty()) return 0.0;
            double v = 0.0;
            std::from_chars(s.data(), s.data() + s.size(), v);
            return v;
        }

        std::size_t parse_sz(std::string_view s) {
            if (s.empty()) return 0;
            std::size_t v = 0;
            std::from_chars(s.data(), s.data() + s.size(), v);
            return v;
        }

    }  // namespace

    std::optional<Tick> parse_tick_line(std::string_view line, TickParseError* err) {
        if (line.empty() || line.starts_with('<')) return std::nullopt;

        std::vector<std::string_view> fields;
        fields.reserve(7);
        std::size_t start = 0;
        while (start <= line.size()) {
            const auto tab = line.find('\t', start);
            if (tab == std::string_view::npos) {
                fields.push_back(line.substr(start));
                break;
            }
            fields.push_back(line.substr(start, tab - start));
            start = tab + 1;
        }

        // Keep empty LAST/VOLUME. Header has 7 columns.
        if (fields.size() < 4) {
            if (err) *err = TickParseError{ 0, std::string(line), "fewer than 4 columns" };
            return std::nullopt;
        }

        Tick t;
        t.m_date = parse_date(fields[0]);
        t.m_time = parse_tod(fields[1]);
        t.m_bid = parse_d(fields[2]);
        t.m_ask = parse_d(fields[3]);
        t.m_last = fields.size() > 4 ? parse_d(fields[4]) : 0.0;
        t.m_volume = fields.size() > 5 ? parse_d(fields[5]) : 0.0;
        t.m_flags = fields.size() > 6 ? parse_sz(fields[6]) : 0;

        if (t.m_bid <= 0.0 || t.m_ask <= 0.0) {
            if (err) *err = TickParseError{ 0, std::string(line), "non-positive bid/ask" };
            return std::nullopt;
        }
        if (t.m_bid > t.m_ask) {
            if (err) *err = TickParseError{ 0, std::string(line), "crossed book" };
            return std::nullopt;
        }
        return t;
    }

    std::vector<Tick> load_ticks_csv(const std::filesystem::path& path,
        std::vector<TickParseError>* errors) {
        std::ifstream in(path);
        std::vector<Tick> out;
        if (!in) {
            if (errors) errors->push_back({ 0, path.string(), "could not open" });
            return out;
        }
        std::string line;
        std::size_t n = 0;
        while (std::getline(in, line)) {
            ++n;
            TickParseError err;
            auto t = parse_tick_line(line, &err);
            if (t) {
                out.push_back(*t);
            }
            else if (errors && !line.empty() && !line.starts_with('<')) {
                err.line = n;
                errors->push_back(std::move(err));
            }
        }
        return out;
    }

}  // namespace map::market_data
