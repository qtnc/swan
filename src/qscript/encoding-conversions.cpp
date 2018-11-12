#include "QValue.hpp"
#include "../include/cpprintf.hpp"
#include<utf8.h>
#include "base64.h"
#include<unordered_map>
#include<vector>
#include<sstream>
#include<iomanip>
using namespace std;

template<class T> struct ostream_iterator_adapter: iterator<output_iterator_tag, T> {
ostream* out;
ostream_iterator_adapter (ostream& o): out(&o) {}
inline ostream_iterator_adapter<T>& operator* () { return *this; }
inline ostream_iterator_adapter<T>& operator++ () { return *this; }
inline ostream_iterator_adapter<T>& operator++ (int unused) { return *this; }
inline ostream_iterator_adapter<T>&  operator= (const T& x) { out->write(reinterpret_cast<const char*>(&x), sizeof(T)); return *this; }
};

template<class T> struct istream_iterator_adapter: iterator<input_iterator_tag, T> {
istream* in;
T cur;
istream_iterator_adapter (istream& in0): in(&in0), cur(0) { ++*this; }
istream_iterator_adapter (): in(nullptr), cur(0)  {}
inline T operator* () {  return cur;  }
inline istream_iterator_adapter<T>& operator++ () { 
in->read(reinterpret_cast<char*>(&cur), sizeof(T)); 
return *this; 
}
inline istream_iterator_adapter<T> operator++ (int unused) { auto copy = *this; ++*this; return copy; }
inline bool operator == (const istream_iterator_adapter<T>& x) { return x.in? in==x.in : !*in; }
inline bool operator != (const istream_iterator_adapter<T>& x) { return !(*this==x); }
inline bool operator < (const istream_iterator_adapter<T>& x) { return *this!=x; }
};


static void identity (istream& in, ostream& out) {
out << in.rdbuf();
}

static void u16to8 (istream& in, ostream& out) {
auto unused = utf8::utf16to8( istream_iterator_adapter<uint16_t>(in), istream_iterator_adapter<uint16_t>(), ostream_iterator_adapter<char>(out));
}

static void u32to8 (istream& in, ostream& out) {
auto unused = utf8::utf32to8( istream_iterator_adapter<uint32_t>(in), istream_iterator_adapter<uint32_t>(), ostream_iterator_adapter<char>(out));
}

static void binaryToU8 (istream& in, ostream& out) {
auto unused = utf8::utf32to8( istream_iterator_adapter<uint8_t>(in), istream_iterator_adapter<uint8_t>(), ostream_iterator_adapter<char>(out));
}

static void u8to16 (istream& in, ostream& out) {
auto unused = utf8::utf8to16(istream_iterator_adapter<char>(in), istream_iterator_adapter<char>(), ostream_iterator_adapter<uint16_t>(out));
}

static void u8to32 (istream& in, ostream& out) {
auto unused = utf8::utf8to32( istream_iterator_adapter<char>(in), istream_iterator_adapter<char>(), ostream_iterator_adapter<uint32_t>(out));
}

static void u8ToBinary (istream& in, ostream& out) {
auto unused = utf8::utf8to32( istream_iterator_adapter<char>(in), istream_iterator_adapter<char>(), ostream_iterator_adapter<uint8_t>(out));
}

static void b64enc (istream& in, ostream& out) {
ostringstream sOut;
sOut << in.rdbuf();
string sIn = sOut.str();
string sRe = base64_encode(reinterpret_cast<const unsigned char*>(sIn.data()), sIn.size());
out << sRe;
}

static void b64dec (istream& in, ostream& out) {
ostringstream sOut;
sOut << in.rdbuf();
string sIn = sOut.str();
string sRe = base64_decode(sIn);
out << sRe;
}

static void hexenc (istream& in, ostream& out) {
char c=0, b[3]={0};
while(in>>c) {
out << setfill('0') << setw(2) << hex << static_cast<uint16_t>(c);
}}

static void hexdec (istream& in, ostream& out) {
char c=0, b[3] = {0};
while(in >> b[0] >> b[1]) {
c = strtol(b, nullptr, 16);
out << c;
}}

unordered_map<string, QS::VM::EncodingConversionFn> QVM::stringToBufferConverters = {
{ "utf8", identity },
{ "utf16", u8to16 },
{ "utf32", u8to32 },
{ "binary", u8ToBinary },
{ "latin1", u8ToBinary },
{ "iso88591", u8ToBinary },
{ "native", u8ToBinary },
{ "base64", b64dec },
{ "hex", hexdec }
}, 
QVM::bufferToStringConverters = {
{ "utf8", identity },
{ "utf16", u16to8 },
{ "utf32", u32to8 },
{ "binary", binaryToU8 },
{ "latin1", binaryToU8 },
{ "iso88591", binaryToU8 },
{ "native", binaryToU8 },
{ "base64", b64enc },
{ "hex", hexenc }
};


