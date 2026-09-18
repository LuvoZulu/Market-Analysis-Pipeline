# Testing MAP

This files outlines what each test file aims to achieve and how it is achieved. 



```
CMakeLists.txt                                          ← replace yours
include/map/tick_csv.h                                  ← new (parse extract)
src/map/tick_csv.cpp                                    ← new
include/technical/indicators/volume_weighted_average_price.h   ← replace empty
src/technical/indicators/volume_weighted_average_price.cpp     ← replace empty
test/CMakeLists.txt                                     ← new
test/helpers.h                                          ← new
test/test_*.cpp                                         ← replace empty test_candlestick.cpp
```

`conanfile.txt` already has `gtest/1.18.0`. Leave it.

## Why CMake changed

Tests cannot link an **executable**. `CandleStickBuilder`’s constructor also starts a
`jthread` that loops on `system_clock::now()` and re-reads `gold.csv` forever, so
tests never construct that class.

`map_core` is the library (everything except `main.cpp`).
`Market_Analysis_Pipeline` is `main.cpp` + `map_core`.
`map_tests` links `map_core` + `GTest::gtest_main`.

## Build

Use Conan `CMakeConfigDeps` + `cmake_layout`. From the repo root:

```bash
conan install . --build=missing -s build_type=Release
cmake --preset conan-default -DBUILD_TESTING=ON
cmake --build --preset conan-release --target map_tests
ctest --preset conan-release --output-on-failure
```

If presets are not generated:

```bash
cmake -S . -B build/Debug \
  -DCMAKE_TOOLCHAIN_FILE=build/Debug/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/Debug -j
ctest --test-dir build/Debug --output-on-failure
```

Filter:

```bash
./build/Debug/test/map_tests --gtest_filter=M1_*
./build/Debug/test/map_tests --gtest_filter=M3_Atr.*
```

Sanitizers:

```bash
cmake --preset conan-debug -DMAP_ENABLE_SANITIZERS=ON -DBUILD_TESTING=ON
```

`gtest_discover_tests` sets `WORKING_DIRECTORY` to the repo root so
`data/gold.csv` resolves.

## What each test file covers

| File | Milestone | Against |
|---|---|---|
| `test_logger.cpp` | M1 | `map::logging::Logger` / spdlog |
| `test_candlestick.cpp` | M1–M2 | `Tick`, `Candlestick`, M1/M5 aggregation contract |
| `test_tick_csv.cpp` | M1 | MT5 CSV: **keep empty LAST/VOLUME columns** |
| `test_atr.cpp` | M3 | `map::indicators::Atr` |
| `test_vwap.cpp` | M3 | `map::indicators::Vwap` (header was empty — filled) |
| `test_range.cpp` | M3 | LUV-10 average range; production header optional |
| `test_m4_momentum.cpp` | M4 | skips until `technical/tools/momentum.h` |
| `test_m5_structure.cpp` | M5 | skips until `technical/tools/market_structure.h` |
| `test_m6_sentiment.cpp` | M6 | skips until trend/session/candidate headers |
| `test_m7_pipeline.cpp` | M7 | gold.csv → M1 → VWAP, deterministic replay |

M4–M6 use `__has_include`. They **skip**, they don’t fail, until you add those headers.

## Bugs these tests are written to catch

These are in the tree tree today. The tests encode the **correct** contract, so they
will fail until you fix the production code — that is the point.

1. **`build_tick` drops empty CSV fields.**  
   `views::filter(!empty)` collapses `LAST` and `VOLUME`, so FLAGS becomes field 4
   and bid/ask are assigned swapped. `test_tick_csv.cpp` parses
   `...\t4091.771\t4092.251\t\t\t6` and expects bid=4091.771, ask=4092.251, flags=6.

2. **`Atr::get_average(data, target_date)` returns 0 on a same-day series.**  
   The loop is `while (curr->m_date != target_date)`, so gold.csv (all 2026-07-26)
   never contributes a TR. `M3_Atr.DateWindowIncludesTheTargetDay` requires the
   target day to be **included**.

3. **Fourth ATR overload uses `to_time` on the skip path**  
   `m_time < to_time` instead of `from_time`, and it is ORed without the date.
   `M3_Atr.DateTimeWindowUsesFromTimeNotToTime`.

4. **Tick volume is `tick.m_volume` which `build_tick` never sets.**  
   OTC volume is empty. M1 aggregation contract: `m_tick_volume = count of ticks`.
   Keep `m_volume` as 0 until you have real volume.

5. **Wall clock.**  
   `make_candlesticks` waits on `system_clock::now()`. Bars must close on **tick
   timestamps** (`m_date` + `m_time`). `M7_Pipeline.DoesNotUseWallClock`.

6. **Constructor starts a live thread** with a hardcoded `..\\..\\data\\gold.csv`.  
   Tests never construct `CandleStickBuilder`. Feed ticks in; don’t start IO in
   the constructor.

## Naming for M4–M7 (matches tree)

```
include/technical/tools/momentum.h          # swings + liquidity
include/technical/tools/market_structure.h  # FVG, OB, S/R, CHoCH/BOS
include/technical/tools/trend.h
include/technical/tools/session.h
include/technical/tools/candidate.h
include/map/pipeline.h
src/technical/tools/*.cpp
```

Namespace: `map::technical`, same `m_` members, `[[nodiscard]]` (your header has
`[[nodisgard]]` — typo).


## Make this codebase effective

**Split feed from aggregation.** `CandleStickBuilder` currently parses a file, owns six timeframe vectors, waits on the wall clock, and starts a thread in the constructor. That is four jobs. Make:

* `parse_tick_line` / `load_ticks_csv` (already extracted)
* `aggregate(ticks, Timeframe) -> vector<Candlestick>` as a pure function
* a feeder that *pushes* ticks (MT5 socket later; CSV now)
* `main` as the only place that starts threads

Until then you cannot test M2 without hanging.

**Time is on the tick, not on the machine.** ISSUES.md #4 is already a bug, not a possibility. `floor<minutes>(system_clock::now())` will never match `gold.csv`’s 2026-07-26 22:01. Store `sys_time` as `m_date + m_time` (or one `sys_time_point`) and close bars when the *tick’s* timestamp crosses the boundary.

**Do not skip empty MT5 columns.** The filter on `views::split('\t')` is why bid/ask/flags are wrong. Split on tab and keep empties. LAST empty ⇒ mid price. VOLUME empty ⇒ `m_tick_volume = 1` per tick, `m_volume = 0`.

**ATR: pick one definition and document it.** You currently compute a simple mean of true range over a date filter. Wilder ATR (period 14 RMA) is what traders mean by ATR. Either:

* rename to `mean_true_range` and keep the window overloads, or
* implement Wilder in `Atr` and keep the window as a *filter*, not the averaging method.

Same-day gold data must not return 0.

**VWAP is “the price we have to beat” (LUV-9).** Weight by `m_tick_volume`. Never by the empty MT5 VOLUME column.

**`get_candlesticks()` returns M30.** Callers will think they have M1. One getter per timeframe, or `get_candlesticks(Timeframe)`.

**Hardcoded `..\\..\\data\\gold.csv`.** Pass the path in. Use `std::filesystem` so it works on Linux CI and your Windows box. Constructor must not open files.

**Logger: `logs/` is relative to cwd**, not the binary. You’ll get random log files depending on how you launch. Take a directory in `init(name, log_dir)`. Bounded rotating files are already correct (8 × 1MB).

**Make indicators `const`.** `get_average(vector&)` cannot mutate the bars — take `const vector&` and `const year_month_day&`. Tests had to declare non-const `auto day` only because of your signature.

**One library, one binary, one test binary.** Add `-Wall -Wextra -Werror` once the `[[nodisgard]]` typo is fixed. Run ASan/UBSan on `map_tests`.

**M4–M6 have zero Linear issues and 0% progress while M7 is due today.** Open issues for swings, FVG, BOS, sessions, candidates. Don’t start M7 integration of stages that don’t exist.

**Gold.csv as a golden.** `M7_Pipeline.GoldCsvToM1ToVwapAtrIsDeterministic` is the seed. When M4 exists, assert swing count on this file too. That is more valuable than another synthetic path.

**Ring buffer later.** `std::queue<Tick>` plus six `vector<Candlestick>` is fine until the pipeline is pure and tested. Don’t block M2 on the Ring Buffers project.
