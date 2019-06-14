#include "../vm/VM.hpp"
#include "../../include/cpprintf.hpp"
#include<utf8.h>

#ifndef NO_BUFFER
#include "base64.h"
#endif
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

template<class T, class F> struct ostream_iterator_functor_adapter: iterator<output_iterator_tag, T> {
ostream* out;
F func;
ostream_iterator_functor_adapter (ostream& o, const F& f): out(&o), func(f)  {}
inline ostream_iterator_functor_adapter<T,F>& operator* () { return *this; }
inline ostream_iterator_functor_adapter<T,F>& operator++ () { return *this; }
inline ostream_iterator_functor_adapter<T,F>& operator++ (int unused) { return *this; }
inline ostream_iterator_functor_adapter<T,F>&  operator= (const T& x) { func(*out, x); return *this; }
};

template<class T, class F> struct istream_iterator_functor_adapter: iterator<input_iterator_tag, T> {
istream* in;
F func;
T cur;
istream_iterator_functor_adapter (istream& in0, const F& f): in(&in0), cur(0), func(f) { ++*this; }
istream_iterator_functor_adapter (): in(nullptr), cur(0), func(nullptr)   {}
inline T operator* () {  return cur;  }
inline istream_iterator_functor_adapter<T,F>& operator++ () { 
cur = func(*in);
return *this; 
}
inline istream_iterator_functor_adapter<T,F> operator++ (int unused) { auto copy = *this; ++*this; return copy; }
inline bool operator == (const istream_iterator_functor_adapter<T,F>& x) { return x.in? in==x.in : !*in; }
inline bool operator != (const istream_iterator_functor_adapter<T,F>& x) { return !(*this==x); }
inline bool operator < (const istream_iterator_functor_adapter<T,F>& x) { return *this!=x; }
};

template<class I> static inline uint32_t u16next (I& in) {
char b[4] = {0};
uint16_t a[2] = {0};
a[0] = *in;
try {
utf8::utf16to8(a, a+1, b);
} catch (utf8::invalid_utf16& z) {
a[1] = *++in;
utf8::utf16to8(a, a+2, b);
}
return utf8::peek_next(b, b+4);
}

static void identity (istream& in, ostream& out) {
out << in.rdbuf();
}

static void u8to8 (istream& in, ostream& out, int mode) {
if (mode==0) out << in.rdbuf();
else if (mode==1) {
istream_iterator_adapter<char> uIn(in);
istream_iterator_adapter<char> uEnd;
ostream_iterator_adapter<char> uOut(out);
utf8::append(utf8::next(uIn, uEnd), uOut);
}
else if (mode==2) {
string s;
getline(in, s);
out << s;
}}

static void u16to8 (istream& in, ostream& out, int mode) {
istream_iterator_adapter<uint16_t> uIn(in);
istream_iterator_adapter<uint16_t> uEnd;
ostream_iterator_adapter<char> uOut(out);
if (mode==0) utf8::utf16to8( uIn, uEnd, uOut);
else if (mode==1) utf8::append(u16next(uIn), uOut);
else if (mode==2) {
uint32_t n;
while(in && (n=u16next(uIn))>0 && n!='\n') {
if (n!='\n' && n!='\r') utf8::append(n, uOut);
if (n!=0 && n!='\n') ++uIn;
}}}

static void u32to8 (istream& in, ostream& out, int mode) {
istream_iterator_adapter<uint32_t> uIn(in);
istream_iterator_adapter<uint32_t> uEnd;
ostream_iterator_adapter<char> uOut(out);
if (mode==0) utf8::utf32to8( uIn, uEnd, uOut);
else if (mode==1) utf8::append(*uIn, uOut);
else if (mode==2) {
uint32_t n;
while(in && (n=*uIn)>0 && n!='\n') {
if (n!='\n' && n!='\r') utf8::append(n, uOut); 
if (n!=0 && n!='\n') ++uIn;
}}}

static void binaryToU8 (istream& in, ostream& out, int mode) {
if (mode==0) utf8::utf32to8( istream_iterator_adapter<uint8_t>(in), istream_iterator_adapter<uint8_t>(), ostream_iterator_adapter<char>(out));
else if (mode==1) {
ostream_iterator_adapter<char> uOut(out);
char c;
in >> c;
utf8::append(static_cast<uint8_t>(c), uOut);
}
else if (mode==2) {
string s;
getline(in, s);
utf8::utf32to8( reinterpret_cast<const uint8_t*>(s.data()), reinterpret_cast<const uint8_t*>(s.data() + s.size()), ostream_iterator_adapter<char>(out));
}}

static void u8to16 (istream& in, ostream& out) {
auto unused = utf8::utf8to16(istream_iterator_adapter<char>(in), istream_iterator_adapter<char>(), ostream_iterator_adapter<uint16_t>(out));
}

static void u8to32 (istream& in, ostream& out) {
auto unused = utf8::utf8to32( istream_iterator_adapter<char>(in), istream_iterator_adapter<char>(), ostream_iterator_adapter<uint32_t>(out));
}

static void u8ToBinary (istream& in, ostream& out) {
auto unused = utf8::utf8to32( istream_iterator_adapter<char>(in), istream_iterator_adapter<char>(), ostream_iterator_adapter<uint8_t>(out));
}

#ifndef NO_BUFFER
static void b64enc (istream& in, ostream& out, int mode) {
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

static void hexenc (istream& in, ostream& out, int mode) {
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

static void urlenc (istream& in, ostream& out, int mode) {
string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_/,;&?!@";
char c=0;
while(in.read(&c,1)) {
if (c==' ') out << '+';
else if (string::npos!=chars.find(c)) out << c;
else out << '%' << format("%0$2X", static_cast<uint16_t>(c&0xFF));
}}

static void urldec (istream& in, ostream& out) {
char c=0, b[3] = {0};
while (in.read(&c,1)) {
if (c=='%') {
in >> b[0] >> b[1];
c = strtol(b, nullptr, 16);
}
else if (c=='+') c=' ';
out << c;
}}

static void jsonenc (istream& in, ostream& out, int mode) {
auto encfunc = [](ostream& out, uint16_t x) {
if (x==34) out << "\\\"";
else if (x==39) out << "\\'";
else if (x==92) out << "\\\\"; 
else if (x>=32 && x<127) out << static_cast<char>(x);
else print(out, "\\u%0$4x", x);
};
typedef istream_iterator_adapter<char> incvt;
typedef ostream_iterator_functor_adapter<uint16_t, std::function<void(ostream&,uint16_t)>> outcvt;
auto unused = utf8::utf8to16(incvt(in), incvt(), outcvt(out, encfunc));
}

static void jsondec (istream& in, ostream& out) {
auto decfunc = [](istream& in) -> uint16_t {
char c;
in >> c;
if (c=='\\') {
in >> c;
switch(c){
case 'b': return '\b'; break;
case 'f': return '\f'; break;
case 'n': return '\n'; break;
case 'r': return '\r'; break;
case 't': return '\t'; break;
case 'v': return '\v'; break;
case 'u': {
char buf[5] = {0};
in.read(buf, 4);
int re = 0;
sscanf(buf, "%04x", &re);
return re;
}break;
case 'x': {
char buf[3] = {0};
in.read(buf, 2);
int re = 0;
sscanf(buf, "%02x", &re);
return re;
}break;
default: return static_cast<uint8_t>(c);
}}
else return static_cast<uint8_t>(c);
};
istream_iterator_functor_adapter<uint16_t, std::function<uint16_t(istream&)>> uIn(in, decfunc), uEnd;
ostream_iterator_adapter<char> uOut(out);
utf8::utf16to8( uIn, uEnd, uOut);
}
#endif

unordered_map<string, Swan::VM::EncodingConversionFn> QVM::stringToBufferConverters = {
{ "utf8", identity },
{ "utf16", u8to16 },
{ "utf32", u8to32 },
{ "binary", u8ToBinary },
{ "latin1", u8ToBinary },
{ "iso88591", u8ToBinary },
{ "native", u8ToBinary }
#ifndef NO_BUFFER
, { "base64", b64dec },
{ "hex", hexdec },
{ "urlencoded", urldec },
{ "json", jsondec }
#endif
};

unordered_map<string, Swan::VM::DecodingConversionFn>  QVM::bufferToStringConverters = {
{ "utf8", u8to8 },
{ "utf16", u16to8 },
{ "utf32", u32to8 },
{ "binary", binaryToU8 },
{ "latin1", binaryToU8 },
{ "iso88591", binaryToU8 },
{ "native", binaryToU8 }
#ifndef NO_BUFFER
, { "base64", b64enc },
{ "hex", hexenc },
{ "urlencoded", urlenc },
{ "json", jsonenc }
#endif
};


