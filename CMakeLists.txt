cmake_minimum_required(VERSION 3.28)
project(Market_Analysis_Pipeline
    DESCRIPTION "The Market Analysis Pipeline converts raw ticks into trade candidates such as candlesticks"
    VERSION 0.0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MAP_ENABLE_SANITIZERS "ASan+UBSan on map_tests" OFF)

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# Library of everything except main() so tests can link without starting
# CandleStickBuilder's background thread.
add_library(map_core
    src/map/Candlestick.cpp
    src/map/tick_csv.cpp
    src/technical/indicators/average_true_range.cpp
    src/technical/indicators/volume_weighted_average_price.cpp
)
target_include_directories(map_core
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
)
target_link_libraries(map_core
    PUBLIC
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

add_executable(Market_Analysis_Pipeline
    src/main.cpp
)
target_link_libraries(Market_Analysis_Pipeline
    PRIVATE
        map_core
)

include(CTest)
if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    add_subdirectory(test)
endif()
