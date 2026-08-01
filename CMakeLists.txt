cmake_minimum_required(VERSION 3.28)

project(Market_Analysis_Pipeline DESCRIPTION "The Market Analysis Pipeline converts raw ticks into trade candidates such as candlesticks"
        VERSION 0.0.1 
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)


