#ifdef __WIN32
#include<windows.h>
#include<cstdint>

double nativeClock () {
static uint64_t freq = 0;
uint64_t time = 0;
if (!freq) QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&time));
return static_cast<double>(time) / static_cast<double>(freq);
}

#else
#include<ctime>

double nativeClock () {
return clock() / CLOCKS_PER_SEC;
}

#endif

