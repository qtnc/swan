#ifndef NO_BUFFER
#include "VM.hpp"
#include "../../include/SwanCodec.hpp"
using namespace std;
namespace io = boost::iostreams;

template<class Sink, class T>
struct sink_output_iterator {
Sink& sink;
sink_output_iterator (const sink_output_iterator<Sink,T>&) = default;
sink_output_iterator (Sink& s): sink(s) {}
inline sink_output_iterator<Sink,T>& operator++ () { return *this; }
inline sink_output_iterator<Sink,T>& operator++ (int unused) { return *this; }
inline sink_output_iterator<Sink,T>& operator* () { return *this; }
inline sink_output_iterator<Sink,T>& operator= (const sink_output_iterator<Sink,T>& unused) { return *this; };
inline sink_output_iterator<Sink,T>& operator= (const T& x) {
io::write(sink, reinterpret_cast<const char*>(&x), sizeof(T));
return *this;
}};

struct bintou8: io::multichar_output_filter {
template <class Sink> std::streamsize write (Sink& sink, const char* s, std::streamsize n) {
sink_output_iterator<Sink,uint8_t> out(sink);
for (const uint8_t* it=reinterpret_cast<const uint8_t*>(s), *end=it+n; it<end; ++it) {
utf8::append(*it, out);
}
return n;
}
};

struct u8tobin: io::multichar_output_filter {
template <class Sink> std::streamsize write (Sink& sink, const char* s, std::streamsize n) {
sink_output_iterator<Sink,uint8_t> out(sink);
const char* e = utf8::find_invalid(s, s+n);
utf8::utf8to32(s, e, out);
return e-s;
}
};

struct u16to8: io::multichar_output_filter {
template <class Sink> std::streamsize write (Sink& sink, const char* s, std::streamsize n) {
sink_output_iterator<Sink,uint8_t> out(sink);
const uint16_t* t = reinterpret_cast<const uint16_t*>(s), *e = t+n/2;
utf8::utf16to8(t, e, out);
return n;
}
};

struct u8to16: io::multichar_output_filter {
template <class Sink> std::streamsize write (Sink& sink, const char* s, std::streamsize n) {
sink_output_iterator<Sink,uint16_t> out(sink);
const char* e = utf8::find_invalid(s, s+n);
utf8::utf8to16(s, e, out);
return e-s;
}
};

struct dectohex: io::multichar_output_filter {
template <class Sink> std::streamsize write (Sink& sink, const char* s, std::streamsize n) {
for (const char *it=s, *end=it+n; it<end; ++it) {
string hex = format("%0$2x", static_cast<uint16_t>(*reinterpret_cast<const uint8_t*>(it)));
io::write(sink, hex.data(), hex.size());
}
return n;
}
};

struct hextodec: io::multichar_output_filter {
template <class Sink> std::streamsize write (Sink& sink, const char* s, std::streamsize n) {
n&=~1;
char b[3] = {0};
uint8_t c;
for (const char *it=s, *end=it+n; it<end; it+=2) {
memcpy(b, it, 2);
c = strtoul(b, nullptr, 16);
io::put(sink, c);
}
return n;
}
};

struct urlencode: io::multichar_output_filter {
static const string chars;
template <class Sink> std::streamsize write (Sink& sink, const char* s, std::streamsize n) {
for (const char *it=s, *end=it+n; it<end; ++it) {
if (chars.find(*it)!=string::npos) io::put(sink, *it);
else {
string str = format("%%%0$2X", static_cast<uint16_t>(*reinterpret_cast<const uint8_t*>(it)));
io::write(sink, str.data(), str.size());
}}
return n;
}
};
const string urlencode::chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.,";

struct urldecode: io::multichar_output_filter {
template <class Sink> std::streamsize write (Sink& sink, const char* s, std::streamsize n) {
char b[3] = {0};
uint8_t c;
for (const char *it=s, *end=it+n; it<end; ++it) {
switch(*it){
case '%':
if (end-it<2) return it-s;
memcpy(b, ++it, 2);
c = strtoul(b, nullptr, 16);
io::put(sink, c);
it+=2;
break;
case '+': io::put(sink, ' '); break;
default: io::put(sink, *it); break;
}}
return n;
}
};

string Swan::Codec::encode (const string& s) {
string re;
io::filtering_ostream out;
transcode(out, true);
out.push(io::back_inserter(re));
io::write(out, s.data(), s.size());
io::flush(out);
return re;
}

string Swan::Codec::decode (const string& s) {
string re;
io::filtering_ostream out;
transcode(out, false);
out.push(io::back_inserter(re));
io::write(out, s.data(), s.size());
io::flush(out);
return re;
}

Swan::Codec& export Swan::VM::findCodec (const string& name) {
auto it = QVM::codecs.find(name);
if (it==QVM::codecs.end()) error<invalid_argument>("Couldn't find codec '%s'", name);
return *it->second;
}

void export Swan::VM::registerCodec (const std::string& name, Swan::Codec* codec) {
QVM::codecs[name] = shared_ptr<Swan::Codec>(codec);
}

struct NopCodec: Swan::Codec {
virtual int getFlags () override { return  CFE_DECODE_VALID_STRING; }
virtual void transcode (io::filtering_ostream& out, bool encode) override { }
virtual void transcode (io::filtering_istream& out, bool encode) override { }
};

template<class Enc, class Dec, int Flg = CFE_DECODE_VALID_STRING> 
struct FltCodec: Swan::Codec {
virtual int getFlags () override { return Flg; }
virtual void transcode (io::filtering_ostream& out, bool encode) override {
if (encode) out.push(Enc());
else out.push(Dec());
}
virtual void transcode (io::filtering_istream& in, bool encode) override {
if (encode) in.push(io::invert(Enc()));
else in.push(io::invert(Dec()));
}
};

unordered_map<string, shared_ptr<Swan::Codec>> QVM::codecs = {
{ "utf8", make_shared<NopCodec>() },
{ "binary", make_shared<FltCodec<u8tobin, bintou8>>() },
{ "utf16", make_shared<FltCodec<u8to16, u16to8>>() }
, { "hex", make_shared<FltCodec<dectohex, hextodec, CFE_ENCODE_VALID_STRING>>() }
, { "urlencoded", make_shared<FltCodec<urlencode, urldecode, CFE_ENCODE_VALID_STRING>>() }
};

#endif


