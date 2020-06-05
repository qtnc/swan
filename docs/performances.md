# Performances
I tried to compare Swan with a few other programming languages. Globally I'm a bit faster than python 3.6, but slower than wren, and much slower than lua.

I took the scripts that have been used to measure wren's performance with a few minor changes. These scripts are in the benchmark directory.

Script/Language | Swan | Wren | Python 3.6 |  php 7.2.9 | node 7.5.0 | LuaJIT -joff | LuaJIT
-----|-----|-----|-----|-----|----------|-----|
method_call | 0.575 | 0.503 | 3.622 | 1.447 | 0.124 | 0.833 | 0.030
binary_trees | 0.869 | 0.543 | 0.770 | - | 0.154  | 0.224 | 0.216
Fib | 0.480 |  0.563 | 1.591 | 0.533 | 0.077 | 0.234 |  0.049
for | 0.449 |  0.465 |  0.998 | 0.389 | 0.139 | 0.094 | 0.083
list_crunch | 0.115 | - | 0.059 | 2.595 | 0.538 | - | - 
list_insert | 0.879 | 0.856 | 0.512 | 11.50 | 0.434 | 2.850 | 2.850
map_numeric | 0.841 | 0.759 | 0.705 | 1.570 | 0.155 | 0.450 | 0.200
map_string | 0.214 | 0.216 | 0.169 | 0.081 | 0.216 | 0.104 | 0.077
string_equals | 0.110 | 0.502 | 0.808 | 0.055 | 0.234 | 0.055 | 0.004
string_equals_2 | 0.661 | 0.442 |  1.037 | 0.289 | 0.060 | 0.084 | 0.009
