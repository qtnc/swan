@echo off
rem g++ -s -O3 -std=gnu++14 -w *.cpp ../qscript/*.cpp ../cpprintf/*.cpp -fno-gcse -o qs.exe -lboost_regex_1_61_0 -DUSE_COMPUTED_GOTO
g++ -s -O3 -std=gnu++14 -w *.cpp ../qscript/*.cpp ../cpprintf/*.cpp -o qs.exe -lboost_regex_1_61_0