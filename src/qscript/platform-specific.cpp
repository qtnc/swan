#include "QValue.hpp"
#include<string>
#include<utf8.h>
using namespace std;

#ifdef __WIN32
#include<windows.h>

string winConvertEncoding (const char* begin, const char* end, int inCP, int outCP) {
if (!(end-begin)) return "";
int wideLen = MultiByteToWideChar(inCP, MB_PRECOMPOSED, begin, end-begin, nullptr, 0);
wstring wide(wideLen, '\0');
MultiByteToWideChar(inCP, MB_PRECOMPOSED, begin, end-begin, const_cast<wchar_t*>(wide.data()), wideLen);
int outLen = WideCharToMultiByte(outCP, 0, wide.data(), wide.size(), 0, 0, 0, nullptr);
string out(outLen, '\0');
WideCharToMultiByte(outCP, 0, wide.data(), wide.size(), const_cast<char*>(out.data()), outLen, 0, nullptr);
return out;
}

void winConvertEncoding (istream& in, ostream& out, int inCP, int outCP) {
ostringstream sOut;
sOut << in.rdbuf();
string sIn = sOut.str();
string sRe = winConvertEncoding(sIn.data(), sIn.data()+sIn.size(), inCP, outCP);
out << sRe;
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

double nativeClock () {
return clock() / CLOCKS_PER_SEC;
}

#endif

void initPlatformEncodings () {
#define D(N,F) QVM::bufferToStringConverters[#N] = F;
#define E(N,F) QVM::stringToBufferConverters[#N] = F;
//nothing for the moment
#undef D
#undef E

#ifdef __WIN32
#define C(N,E) \
QVM::bufferToStringConverters[#N] = [](istream& in, ostream& out){ winConvertEncoding(in, out, E, CP_UTF8); }; \
QVM::stringToBufferConverters[#N] = [](istream& in, ostream& out){ return winConvertEncoding(in, out, CP_UTF8, E); };
C(native, CP_ACP)
C(oem, CP_OEMCP)
#undef C
#endif
}

