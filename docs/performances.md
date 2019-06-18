# Performances
I tried to compare Swan with a few other programming languages. Globally I'm a bit faster than python 3.6, but slower than wren, and much slower than lua.

I took the scripts that have been used to measure wren's performance with a few minor changes. These scripts are in the benchmark directory.

Script/Language | Swan | Wren | Python 3.6 |  LuaJIT -joff | LuaJIT
-----|-----|-----|-----|-----|-----
method_call | 0.904 | 0.503 | 3.622 | 0.833 | 0.030
binary_trees | 0.777 | 0.543 | 0.770 | 0.224 | 0.216
Fib | 0.778 |  0.563 | 1.591 | 0.234 |  0.049
for | 0.480 |  0.465 |  0.998 | 0.094 | 0.083
map_numeric | 0.736 | 0.759 | 0.705 | 0.450 | 0.200
map_string | 0.208 | 0.216 | 0.169 | 0.104 | 0.077
string_equals | 0.664 | 0.502 | 0.808 | 0.055 | 0.004
