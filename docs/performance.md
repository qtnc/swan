# Performances
I tried to compare Swan with a few other programming languages and in fact, I'm globally quite slow. I don't know well how I could improve certain results.

I took the scripts that have been used to measure wren's performance. These scripts are in the benchmark directory.

&nbsp; | Swan | Wren | Python 3.6 |  LuaJIT -joff | LuaJIT
-----|-----|-----|-----|-----|-----
method_call | 0.384 | 0.210 | 1.48 | 0.352 | 0.020
binary_trees | 0.978 | 0.543 | 0.770 | 0.224 | 0.216
Fib | 0.787 |  0.563 | 1.59 | 0.234 |  0.049
for | 0.354 |  0.254 |  0.565 | 0.035 | 0.027
map_numeric | 1.16 | 0.759 | 0.705 | 0.450 | 0.200
map_string | 0.309 | 0.216 |0.169 | 0.104 | 0.077
string_equals | 0.715 | 0.502 | 0.808 | 0.055 | 0.004
