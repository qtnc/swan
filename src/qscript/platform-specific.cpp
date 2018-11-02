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

string nativeToUTF8 (const char* begin, const char* end) {
// Assume the native encoding is ISO-8859-1/Latin-1/CP1252
string re;
auto out = back_inserter(re);
for (const char* c=begin; c<end; c++) utf8::append(static_cast<uint8_t>(*c), out);
return re;
}

string utf8ToNative (const char* begin, const char* end) {
// Assume the native encoding is ISO-8859-1/Latin-1/CP1252
string re;
while(begin<end) {
uint32_t n = utf8::next(begin, end);
re.push_back(static_cast<char>(n));
}
return re;
}

void initPlatformEncodings () {
#define D(N,F) QVM::bufferToStringConverters[#N] = F;
#define E(N,F) QVM::stringToBufferConverters[#N] = F;
E(native, utf8ToNative)
D(native, nativeToUTF8)
E(binary, utf8ToNative)
D(binary, nativeToUTF8)
#undef D
#undef E

#ifdef __WIN32
#define C(N,E) \
QVM::bufferToStringConverters[#N] = [](const char* begin, const char* end){ return winConvertEncoding(begin, end, E, CP_UTF8); }; \
QVM::stringToBufferConverters[#N] = [](const char* begin, const char* end){ return winConvertEncoding(begin, end, CP_UTF8, E); };
C(native, CP_ACP)
C(oem, CP_OEMCP)
#undef C
#endif
}

