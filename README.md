# Market-Analysis-Pipeline
The Market Analysis Pipeline converts raw ticks into trade candidates such as candlesticks. This provides basic functionality such as:
```
- Indicators {Average True Range, Volume-Weighted Average Price)
- Technical Tools {Momentum and Trend Detection}
```

```
└── Market-Analysis-Pipeline/
    ├── README.md
    ├── CMakeLists.txt
    ├── conanfile.txt
    ├── ISSUES.md
    ├── TEST.md
    ├── TODO.md
    ├── data/
    │   └── gold.csv
    ├── include/
    │   ├── logging/
    │   │   └── Logging.h
    │   ├── map/
    │   │   ├── Candlestick.h
    │   │   └── tick_csv.h
    │   └── technical/
    │       └── indicators/
    │           ├── average_true_range.h
    │           └── volume_weighted_average_price.h
    ├── src/
    │   ├── main.cpp
    │   ├── map/
    │   │   ├── Candlestick.cpp
    │   │   └── tick_csv.cpp
    │   └── technical/
    │       └── indicators/
    │           ├── average_true_range.cpp
    │           └── volume_weighted_average_price.cpp
    ├── test/
    │   ├── CMakeLists.txt
    │   ├── helpers.h
    │   ├── test_atr.cpp
    │   ├── test_candlestick.cpp
    │   ├── test_logger.cpp
    │   ├── test_m4_momentum.cpp
    │   ├── test_m5_structure.cpp
    │   ├── test_m6_sentiment.cpp
    │   ├── test_m7_pipeline.cpp
    │   ├── test_range.cpp
    │   ├── test_tick_csv.cpp
    │   └── test_vwap.cpp
    └── .github/
        └── workflows/
            └── ci.yml
```
