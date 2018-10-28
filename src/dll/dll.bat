@echo off
g++ -s -O3 -std=gnu++14 -w ../qscript/*.cpp ../cpprintf/*.cpp -fno-gcse --shared -o qscript.dll -lboost_regex_1_61_0 -Wl,--out-implib,libqscript.a