#ifndef MAP_TICK_CSV_H
#define MAP_TICK_CSV_H

// Extracted from CandleStickBuilder::build_tick so parsing is testable and
// does not depend on a background thread or a hardcoded path.
// MT5 tick export:
//   <DATE>  <TIME>  <BID>  <LAST>  <VOLUME>  <FLAGS>   (tab-separated)
// Empty LAST/VOLUME (OTC) must be kept as empty columns, not dropped.
// Temporary file as we'll be getting our data from a server (starting point of project)

#include "map/Candlestick.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace map::market_data {

    struct TickParseError {
        std::size_t line{ 0 };
        std::string raw;
        std::string why;
    };

    std::optional<Tick> parse_tick_line(std::string_view line, TickParseError* err = nullptr);

    std::vector<Tick> load_ticks_csv(const std::filesystem::path& path,
        std::vector<TickParseError>* errors = nullptr);

}  // namespace map::market_data

#endif
