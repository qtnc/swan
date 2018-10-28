#include<string>
#include<utf8.h>
using namespace std;

#ifdef __WIN32
#include<windows.h>

string winConvertEncoding (const string& in, int inCP, int outCP) {
if (in.empty()) return in;
int wideLen = MultiByteToWideChar(inCP, MB_PRECOMPOSED, in.data(), in.size(), nullptr, 0);
wstring wide(wideLen, '\0');
MultiByteToWideChar(inCP, MB_PRECOMPOSED, in.data(), in.size(), const_cast<wchar_t*>(wide.data()), wideLen);
int outLen = WideCharToMultiByte(outCP, 0, wide.data(), wide.size(), 0, 0, 0, nullptr);
string out(outLen, '\0');
WideCharToMultiByte(outCP, 0, wide.data(), wide.size(), const_cast<char*>(out.data()), outLen, 0, nullptr);
return out;
}

string nativeToUTF8 (const string& s) {
return winConvertEncoding(s, CP_ACP, CP_UTF8);
}

double nativeClock () {
static uint64_t freq = 0;
uint64_t time = 0;
if (!freq) QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&time));
return static_cast<double>(time) / static_cast<double>(freq);
}

#else
#include<ctime>

string nativeToUTF8 (const string& in) {
// Assume the native encoding is ISO-8859-1/Latin-1/CP1252
string re;
auto out = back_inserter(re);
for (char c: in) utf8::append(static_cast<uint8_t>(c), out);
return re;
}

double nativeClock () {
return clock() / CLOCKS_PER_SEC;
}

#endif