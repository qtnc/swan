#include "../include/Swan.hpp"
#include "../include/SwanBinding.hpp"
#include "../include/SwanCodec.hpp"
#include "../include/cpprintf.hpp"
#include<vector>
#include<sstream>
#include<exception>
#include<memory>
#include<cstdlib>
#include<cstdio>
#include<boost/iostreams/device/file.hpp>
using namespace std;
namespace io = boost::iostreams;

typedef vector<string> encspec;

template<class E, class... A> inline void error (const char* fmt, A&&... args) {
string msg = format(fmt, args...);
throw E(msg);
}

template<class Source>
static shared_ptr<io::filtering_istream> buildInput (Source& src, const encspec& encodings) {
auto f = make_shared<io::filtering_istream>();
f->set_auto_close(false);
for (auto it=encodings.rbegin(), end=encodings.rend(); it!=end; ++it) {
Swan::VM::findCodec(*it).transcode(*f, false);
}
f->push(src);
return f;
}

template<class Sink>
shared_ptr<io::filtering_ostream> buildOutput (Sink& snk, const encspec& encodings) {
auto f = make_shared<io::filtering_ostream>();
f->set_auto_close(false);
for (auto& encoding: encodings) Swan::VM::findCodec(encoding).transcode(*f, true);
f->push(snk);
return f;
}

struct Reader {
shared_ptr<io::filtering_istream> in;
encspec encodings;
virtual ~Reader () = default;
virtual bool isBinary () {  return !encodings.size();  }
virtual void close () { in.reset(); }
virtual int read (char* buf, int max) {
in->read(buf, max);
return in? in->gcount() : -1;
}
virtual string readAll () {
ostringstream out;
out << in->rdbuf()  << flush;
return out.str();
}
virtual bool readLine (string& s) { return static_cast<bool>(std::getline(*in,s)); }
template<class Source> void reset (Source& src, const encspec& enc) {
encodings = enc;
in = buildInput(src, encodings);
}};

struct Writer {
shared_ptr<io::filtering_ostream> out;
encspec encodings;
virtual ~Writer() = default;
virtual bool isBinary () { return !encodings.size(); }
virtual void write (const char* buf, size_t length) {  out->write(buf, length); }
virtual void write (const string& s) { write(s.data(), s.size()); }
virtual void flush () { out->flush(); }
virtual void close () { out.reset(); }
template<class Sink> void reset (Sink& snk, const encspec& enc) {
encodings = enc;
out = buildOutput(snk, encodings);
}};

struct MemWriter: Writer {
string data;
io::back_insert_device<string> sink;
MemWriter (const encspec& encodings = {}):
data(), sink(data) 
{ reset(sink, encodings); }
virtual ~MemWriter () = default;
};

struct MemReader: Reader {
string data;
io::array_source source;
MemReader (const string& data0, const encspec& encodings = {}): 
data(data0), source(data.data(), data.size()) 
{ reset(source, encodings); }
virtual ~MemReader () = default;
};

struct FileReader: Reader {
io::file_source source;
FileReader (const string& filename, const encspec& encodings, ios::openmode mode = ios::in):
source(filename, mode) 
{ reset(source, encodings); }
virtual ~FileReader () = default;
};

struct FileWriter: Writer {
io::file_sink sink;
FileWriter (const string& filename, const encspec& encodings, ios::openmode mode):
sink(filename, mode) 
{ reset(sink, encodings); }
virtual ~FileWriter () = default;
};

static void writerWrite (Swan::Fiber& f) {
Writer& w = f.getUserObject<Writer>(0);
if (f.isString(1) && !w.isBinary()) {
string s = f.getString(1);
w.write(s);
}
else if (f.isBuffer(1) && w.isBinary()) {
int length;
const char* buf = f.getBuffer<char>(1, &length);
w.write(buf, length);
}
else if (w.isBinary()) error<invalid_argument>("Writing to a binary writer requires a Buffer");
else error<invalid_argument>("Writing to a text writer requires a String");
}

static void readerRead (Swan::Fiber& f) {
Reader& r = f.getUserObject<Reader>(0);
int num = f.getOptionalNum(1, "size", -1);
if (num<=0) f.setString(0, r.readAll());
else {
std::unique_ptr<char[]> buf = make_unique<char[]>(num);
int count = r.read(&buf[0], num);
if (count<0) f.setUndefined(0);
if (r.isBinary()) f.setBuffer(0, &buf[0], count);
else f.setString(0, string(&buf[0], count));
}}

static void readerReadLine (Swan::Fiber& f) {
Reader& r = f.getUserObject<Reader>(0);
string s;
if (r.readLine(s)) {
if (r.isBinary()) f.setBuffer(0, s.data(), s.size());
else f.setString(0, s);
}
else f.setUndefined(0);
}

static ios::openmode decodeMode (Swan::Fiber& f, int idx, const char* defaultMode) {
string sMode;
if (sMode.empty() && f.getArgCount()>=idx+1 && f.isString(idx)) {
string s = f.getString(idx);
if (s.size()<=3) sMode=s;
}
if (sMode.empty() && f.getArgCount()>=idx+2 && f.isString(idx+1)) {
string s = f.getString(idx+1);
if (s.size()<=3) sMode=s;
}
if (sMode.empty() && f.getArgCount()>=idx+1) {
string s = f.getOptionalString("mode", defaultMode);
if (s.size()<=3) sMode=s;
}
if (sMode.empty()) sMode = defaultMode;
if (f.getArgCount()>=idx+1) {
bool binary = f.getOptionalBool("binary", false);
bool append = f.getOptionalBool("append", false);
if (binary) sMode+='b';
if (append) sMode+='a';
}
ios::openmode mode = ios::in ^ios::in; // 0
for (char c: sMode) {
switch(c){
case 'a': mode |= ios::app; break;
case 'b': mode |= ios::binary; break;
case 'r': mode |= ios::in; break;
case 'w': mode |= ios::out; break;
}}
return mode;
}

static encspec decodeEncoding (Swan::Fiber& f, int idx) {
encspec encs;
if (encs.empty() && f.getArgCount()>=idx+1 && f.isString(idx)) {
string s = f.getString(idx);
if (s.size()>3) encs.push_back(s);
}
if (encs.empty() && f.getArgCount()>=idx+2 && f.isString(idx+1)) {
string s = f.getString(idx+1);
if (s.size()>3) encs.push_back(s);
}
if (encs.empty() && f.getArgCount()>=idx+1) {
string s = f.getOptionalString("encoding", "");
if (s.size()>3) encs.push_back(s);
}
return encs;
}

static void fileReaderConstruct (Swan::Fiber& f) {
string filename = f.getString(1);
auto mode = decodeMode(f, 2, "r");
auto enc = decodeEncoding(f, 2);
f.emplaceUserObject<FileReader>(0, filename, enc, mode);
}

static void fileWriterConstruct (Swan::Fiber& f) {
string filename = f.getString(1);
auto mode = decodeMode(f, 2, "w");
auto enc = decodeEncoding(f, 2);
f.emplaceUserObject<FileWriter>(0, filename, enc, mode);
}

static void memWriterConstruct (Swan::Fiber& f) {
auto enc = decodeEncoding(f, 1);
f.emplaceUserObject<MemWriter>(0, enc);
}

static void memReaderConstruct (Swan::Fiber& f) {
string src;
if (f.isString(1)) src = f.getString(1);
else if (f.isBuffer(1)) {
int length;
const char* buf = f.getBuffer<char>(1, &length);
src = string(buf, length);
}
auto enc = decodeEncoding(f, 2);
f.emplaceUserObject<MemReader>(0, src, enc);
}

static void memWriterValue (Swan::Fiber& f) {
MemWriter& mw = f.getUserObject<MemWriter>(0);
if (mw.isBinary()) f.setBuffer(0, mw.data.data(), mw.data.size());
else f.setString(0, mw.data);
}

static void print (Swan::Fiber& f) {
auto& p = std::cout;
for (int i=0, n=f.getArgCount(); i<n; i++) {
if (i>0) p<<' ';
if (f.isString(i)) p << f.getString(i);
else {
f.pushCopy(i);
f.callMethod("toString", 1);
p << f.getString(-1);
f.pop();
}}
p << endl;
f.setUndefined(0);
}


void registerIO (Swan::Fiber& f) {
f.loadGlobal("Iterable");
f.registerClass<Reader>("Reader", 1);
f.registerMethod("read", readerRead);
f.registerMethod("readLine", readerReadLine);
f.registerMethod("next", readerReadLine);
f.registerMethod("close", METHOD(Reader, close));

f.pushCopy();
f.registerClass<FileReader>("FileReader", 1);
f.registerStaticMethod("()", fileReaderConstruct);
f.pop(); // FileReader

f.pushCopy();
f.registerClass<MemReader>("MemReader", 1);
f.registerStaticMethod("()", memReaderConstruct);
f.pop(); // MemReader

f.pop(); // Reader

f.registerClass<Writer>("Writer");
f.registerMethod("write", writerWrite);
f.registerMethod("flush", METHOD(Writer, flush));
f.registerMethod("close", METHOD(Writer, close));

f.pushCopy();
f.registerClass<FileWriter>("FileWriter", 1);
f.registerStaticMethod("()", fileWriterConstruct);
f.pop(); // FileWriter

f.pushCopy();
f.registerClass<MemWriter>("MemWriter", 1);
f.registerStaticMethod("()", memWriterConstruct);
f.registerMethod("value", memWriterValue);
f.pop(); // MemWriter

f.pop(); // Writer

f.registerFunction("print", print);
}
