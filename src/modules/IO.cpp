#include "IO.hpp"
#include "../include/Swan.hpp"
#include "../include/SwanBinding.hpp"
#include "../include/SwanCodec.hpp"
using namespace std;
namespace io = boost::iostreams;

struct IO {
static Swan::Handle in, out, err;
};
Swan::Handle IO::in, IO::out, IO::err;

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
if (num<=0) {
string s = r.readAll();
if (r.isBinary()) f.setBuffer(0, s.data(), s.size());
else f.setString(0,s);
}
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
int  fd = f.getOptionalNum(1, -1);
string filename = f.getOptionalString(1, "");
auto mode = decodeMode(f, 2, "r");
auto enc = decodeEncoding(f, 2);
if (fd>=0) f.emplaceUserObject<FileReader>(0, fd, enc);
else f.emplaceUserObject<FileReader>(0, filename, enc, mode);
}

static void fileWriterConstruct (Swan::Fiber& f) {
int fd = f.getOptionalNum(1, -1);
string filename = f.getOptionalString(1, "");
auto mode = decodeMode(f, 2, "w");
auto enc = decodeEncoding(f, 2);
if (fd>=0) f.emplaceUserObject<FileWriter>(0, fd, enc);
else f.emplaceUserObject<FileWriter>(0, filename, enc, mode);
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

void print (Swan::Fiber& f) {
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


void swanLoadIO (Swan::Fiber& f) {
f.loadGlobal("Map");
f.call(0);

f.loadGlobal("Iterable");
f.pushNewClass<Reader>("Reader", 1);
f.registerMethod("read", readerRead);
f.registerMethod("readLine", readerReadLine);
f.registerMethod("next", readerReadLine);
f.registerMethod("close", METHOD(Reader, close));
f.registerProperty("position", METHOD(Reader, tell), METHOD(Reader, seek));

f.pushCopy();
f.pushNewClass<FileReader>("FileReader", 1);
f.registerStaticMethod("()", fileReaderConstruct);
f.putInMap(-3, "FileReader");
f.pop(); // FileReader

f.pushCopy();
f.pushNewClass<MemReader>("MemReader", 1);
f.registerStaticMethod("()", memReaderConstruct);
f.putInMap(-4, "MemReader");
f.pop(); // MemReader

f.putInMap(-2, "Reader");
f.pop(); // Reader

f.pushNewClass<Writer>("Writer");
f.registerMethod("write", writerWrite);
f.registerMethod("flush", METHOD(Writer, flush));
f.registerMethod("close", METHOD(Writer, close));
f.registerProperty("position", METHOD(Writer, tell), METHOD(Writer, seek));

f.pushCopy();
f.pushNewClass<FileWriter>("FileWriter", 1);
f.registerStaticMethod("()", fileWriterConstruct);
f.putInMap(-3, "FileWriter");
f.pop(); // FileWriter

f.pushCopy();
f.pushNewClass<MemWriter>("MemWriter", 1);
f.registerStaticMethod("()", memWriterConstruct);
f.registerMethod("value", memWriterValue);
f.putInMap(-3, "MemWriter");
f.pop(); // MemWriter

f.putInMap(-2, "Writer");
f.pop(); // Writer

f.pushNewClass<IO>("IO");
f.registerStaticProperty("in", STATIC_PROPERTY(IO, in));
f.registerStaticProperty("out", STATIC_PROPERTY(IO, out));
f.registerStaticProperty("err", STATIC_PROPERTY(IO, err));
f.putInMap(-2, "IO");
f.pop(); // IO

encspec defenc = { "binary" };
f.pushUndefined();
f.emplaceUserObject<FileReader>(-1, 0, defenc);
IO::in = f.getHandle(-1);
f.pop();

f.pushUndefined();
f.emplaceUserObject<FileWriter>(-1, 1, defenc);
IO::out = f.getHandle(-1);
f.pop();

f.pushUndefined();
f.emplaceUserObject<FileWriter>(-1, 2, defenc);
IO::err = f.getHandle(-1);
f.pop();
}
