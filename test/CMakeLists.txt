include(GoogleTest)

add_executable(map_tests
    test_candlestick.cpp
    test_tick_csv.cpp
    test_logger.cpp
    test_atr.cpp
    test_vwap.cpp
    test_range.cpp
    test_m4_momentum.cpp
    test_m5_structure.cpp
    test_m6_sentiment.cpp
    test_m7_pipeline.cpp
)

target_include_directories(map_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(map_tests PRIVATE map_core GTest::gtest_main)
target_compile_features(map_tests PRIVATE cxx_std_23)

# data/gold.csv is resolved from the repo root.
gtest_discover_tests(map_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DISCOVERY_TIMEOUT 60
    PROPERTIES TIMEOUT 60
)

if(MAP_ENABLE_SANITIZERS)
    target_compile_options(map_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(map_tests PRIVATE -fsanitize=address,undefined)
endif()
