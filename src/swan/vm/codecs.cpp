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

template<class Enc, class Dec> 
struct FltCodec: Swan::Codec {
virtual int getFlags () { return CFE_DECODE_VALID_STRING; }
virtual void transcode (io::filtering_ostream& out, bool encode) {
if (encode) out.push(Enc());
else out.push(Dec());
}
virtual void transcode (io::filtering_istream& in, bool encode) override {
if (encode) in.push(io::invert(Enc()));
else in.push(io::invert(Dec()));
}
};

unordered_map<string, shared_ptr<Swan::Codec>> QVM::codecs = {
{ "binary", make_shared<FltCodec<u8tobin, bintou8>>() },
{ "utf16", make_shared<FltCodec<u8to16, u16to8>>() }
};

#endif


