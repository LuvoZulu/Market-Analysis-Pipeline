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