@echo off
g++ -s -O3 -std=gnu++17 -Wextra *.cpp ../qscript/*.cpp ../cpprintf/*.cpp -fno-gcse -o qs.exe -lboost_regex_1_61_0 -DUSE_COMPUTED_GOTO
rem g++ -s -O3 -std=gnu++17 -Wextra *.cpp ../qscript/*.cpp ../cpprintf/*.cpp -o qs.exe -lboost_regex_1_61_0