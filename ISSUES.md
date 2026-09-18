## Market Analysis Pipeline issues

This file keeps track of all the software decisions I make as temporary that I aim to replace later once I solve my issues. These issue stem from the 2 possible point:

```
- The data feed I am using currently(mt5) does not provide me with full market data as I am currently not trading stocks. (No sense of true volume)
- My current development skills are lacking and once improved will replace the code with issues.
```


###

1. Candlestick Volume:
```
In [Candlestick.cpp](src/map/Candlestick.cpp), mt5 does not give me actual data for the pairs I am trading so I currently have the number of ticks for ticks as volume and n for the nth timeframe candlestick.
```

2. Building Ticks
```
In [Candlestick.h](include/map/Candlestick.h), the build_tick method takes in a path parameter which is a temprary approach to files i currently have locally. Correct approach is probably a server giving us these ticks.
```

3. Hard-coded values
```
In the candlestick class I am using hardcoded values and this is meaningless and not a good practice.
```

4. Time sync with chrono
```
There is a possibility that I face issues with my current usage of chrono. I am using the execution computer as the saw of truth in terms of time when using system_clock::now(). I'll see if this is correct or not as i continue.
```

5. ATR Time disputes
```
In calculating the average true range, in time horizons how do I calculate time accurately when its not provided? Document this thoroughly
```

6. Testing issues
```
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
```