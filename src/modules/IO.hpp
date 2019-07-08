#ifndef ____IO_MODULE__SWAN_____
#define ____IO_MODULE__SWAN_____
#include<optional>
#include "../include/Swan.hpp"
#include "../include/SwanCodec.hpp"
#include "../include/cpprintf.hpp"
#include<vector>
#include<sstream>
#include<exception>
#include<memory>
#include<boost/iostreams/device/file_descriptor.hpp>
using std::string;
using std::vector;
using std::optional;
using std::make_shared;
using std::shared_ptr;
namespace io = boost::iostreams;

typedef vector<string> encspec;

template<class E, class... A> inline void error (const char* fmt, A&&... args) {
string msg = format(fmt, args...);
throw E(msg);
}

template<class Source>
static bool buildInput (Source& src, shared_ptr<io::filtering_istream>& f,  const encspec& encodings) {
if (!f) f = make_shared<io::filtering_istream>();
else while(f->size()) f->pop();
optional<bool> binary;
f->set_auto_close(false);
for (auto it=encodings.rbegin(), end=encodings.rend(); it!=end; ++it) {
auto& codec = Swan::VM::findCodec(*it);
codec.transcode(*f, false);
if (!binary.has_value()) binary = !(codec.getFlags()&CFE_DECODE_VALID_STRING);
}
f->push(src);
return binary.value_or(true);
}

template<class Sink>
static bool buildOutput (Sink& snk, shared_ptr<io::filtering_ostream>& f, const encspec& encodings) {
if (!f) f = make_shared<io::filtering_ostream>();
else while(f->size()) f->pop();
bool binary = true;
f->set_auto_close(false);
for (auto& encoding: encodings) {
auto& codec = Swan::VM::findCodec(encoding);
codec.transcode(*f, true);
binary = codec.getFlags()&CFE_ENCODE_VALID_STRING;
}
f->push(snk);
return binary;
}

struct Reader {
shared_ptr<io::filtering_istream> in;
encspec encodings;
bool binary;
virtual ~Reader () = default;
//virtual Reader* clone () = 0;
virtual bool isBinary () { return binary; }
virtual void close () { in.reset(); }
virtual int read (char* buf, int max) {
in->read(buf, max);
return in? in->gcount() : -1;
}
virtual string readAll () {
std::ostringstream out;
out << in->rdbuf()  << std::flush;
return out.str();
}
virtual bool readLine (string& s) { 
return static_cast<bool>(std::getline(*in,s)); 
}
virtual int tell (int pos) { return in->tellg(); }
virtual void seek (int pos) {
if (pos<0) in->seekg(pos+1, std::ios::end);
else in->seekg(pos, std::ios::beg);
}
template<class Source> void reset (Source& src, const encspec& enc) {
encodings = enc;
binary = buildInput(src, in, encodings);
}};

struct Writer {
shared_ptr<io::filtering_ostream> out;
encspec encodings;
bool binary;
virtual ~Writer() = default;
//virtual Writer* clone () = 0;
virtual bool isBinary () { return binary; }
virtual void write (const char* buf, size_t length) {  out->write(buf, length); }
virtual void write (const string& s) { write(s.data(), s.size()); }
virtual void flush () { out->flush(); }
virtual void close () { out.reset(); }
virtual int tell () { return out->tellp(); }
virtual void seek (int pos) {
if (pos<0) out->seekp(pos+1, std::ios::end);
else out->seekp(pos, std::ios::beg);
}
template<class Sink> void reset (Sink& snk, const encspec& enc) {
encodings = enc;
binary = buildOutput(snk, out, encodings);
}};

struct MemWriter: Writer {
string data;
io::back_insert_device<string> sink;
MemWriter (const encspec& encodings = {}):
data(), sink(data) 
{ reset(sink, encodings); }
virtual ~MemWriter () = default;
//virtual Writer* clone () override { return new MemWriter(*this); }
};

struct MemReader: Reader {
string data;
io::array_source source;
MemReader (const string& data0, const encspec& encodings = {}): 
data(data0), source(data.data(), data.size()) 
{ reset(source, encodings); }
virtual ~MemReader () = default;
//virtual Reader* clone () override { return new MemReader(*this); }
};

struct FileReader: Reader {
io::file_descriptor_source source;
FileReader (const string& filename, const encspec& encodings, std::ios::openmode mode = std::ios::in):
source(filename, mode) 
{ reset(source, encodings); }
FileReader (int fd, const encspec& encodings, io::file_descriptor_flags flags = io::file_descriptor_flags::never_close_handle):
source(fd, flags)
{ reset(source, encodings); }
virtual ~FileReader () = default;
//virtual Reader* clone () override { return new FileReader(*this); }
};

struct FileWriter: Writer {
io::file_descriptor_sink sink;
FileWriter (const string& filename, const encspec& encodings, std::ios::openmode mode = std::ios::out):
sink(filename, mode) 
{ reset(sink, encodings); }
FileWriter (int fd, const encspec& encodings, io::file_descriptor_flags flags = io::file_descriptor_flags::never_close_handle):
sink(fd, flags)
{ reset(sink, encodings); }
virtual ~FileWriter () = default;
//virtual Writer* clone () override { return new FileWriter(*this); }
};

#endif
