#include "QValue.hpp"
#include "../include/cpprintf.hpp"
#include<utf8.h>
#include "base64.h"
#include<unordered_map>
#include<vector>
#include<sstream>
using namespace std;

template<class T> struct binary_ostream_iterator: iterator<output_iterator_tag, T> {
ostream& out;
binary_ostream_iterator (ostream& o): out(o) {}
inline binary_ostream_iterator<T>& operator* () { return *this; }
inline binary_ostream_iterator<T>& operator++ () { return *this; }
inline binary_ostream_iterator<T>& operator++ (int unused) { return *this; }
inline binary_ostream_iterator<T>&  operator= (const T& x) { out.write(reinterpret_cast<const char*>(&x), sizeof(T)); return *this; }
};

static string identity (const char* begin, const char* end) {
return string(begin, end);
}

static string u16to8 (const char* begin, const char* end) {
string s;
auto out = back_inserter(s);
utf8::utf16to8( reinterpret_cast<const uint16_t*>(begin), reinterpret_cast<const uint16_t*>(end), out);
return s;
}

static string u32to8 (const char* begin, const char* end) {
string s;
auto out = back_inserter(s);
utf8::utf32to8( reinterpret_cast<const uint32_t*>(begin), reinterpret_cast<const uint32_t*>(end), out);
return s;
}

static string u8to16 (const char* begin, const char* end) {
ostringstream out;
utf8::utf8to16(begin, end, binary_ostream_iterator<uint16_t>(out));
return out.str();
}

static string u8to32 (const char* begin, const char* end) {
ostringstream out;
utf8::utf8to32(begin, end, binary_ostream_iterator<uint32_t>(out));
return out.str();
}

static string b64enc (const char* begin, const char* end) {
return base64_encode(reinterpret_cast<const unsigned char*>(begin), end-begin);
}

static string b64dec (const char* begin, const char* end) {
return base64_decode(string(begin, end));
}

unordered_map<string, QS::VM::EncodingConversionFn> QVM::stringToBufferConverters = {
{ "utf8", identity },
{ "utf16", u8to16 },
{ "utf32", u8to32 },
{ "base64", b64dec }
}, 
QVM::bufferToStringConverters = {
{ "utf8", identity },
{ "utf16", u16to8 },
{ "utf32", u32to8 },
{ "base64", b64enc }
};


