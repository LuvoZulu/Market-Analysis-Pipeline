cmake_minimum_required(VERSION 3.28)

project(Market_Analysis_Pipeline
    DESCRIPTION "The Market Analysis Pipeline converts raw ticks into trade candidates such as candlesticks"
    VERSION 0.0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

add_executable(Market_Analysis_Pipeline
    src/main.cpp
    src/map/Candlestick.cpp
    src/technical/indicators/average_true_range.cpp
    src/technical/indicators/volume_weighted_average_price.cpp
)

target_include_directories(Market_Analysis_Pipeline
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(Market_Analysis_Pipeline
    PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)