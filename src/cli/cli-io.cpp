#include "../include/QScript.hpp"
#include "../include/QScriptBinding.hpp"
#include "../include/cpprintf.hpp"
#include<iostream>
#include<sstream>
#include<fstream>
#include<exception>
#include<memory>
using namespace std;

struct IO {
QS::VM::EncodingConversionFn encoder;
QS::VM::DecodingConversionFn decoder;
shared_ptr<istream> in;
shared_ptr<ostream> out;
IO (shared_ptr<istream> i, const QS::VM::DecodingConversionFn& c = nullptr): in(i), out(nullptr), encoder(nullptr), decoder(c) {}
IO (shared_ptr<ostream> o, const QS::VM::EncodingConversionFn& c = nullptr): in(nullptr), out(o), encoder(c), decoder(nullptr)  {}
void flush () {
if (out && *out) *out << std::flush;
}
void close () { 
in=nullptr;
out=nullptr;
}
int tell () { return in && *in ? static_cast<int>(in->tellg()) : -1; }
int seek (int pos, optional<bool> absolute) {
bool abs = absolute.value_or(false);
if (!in || !*in) return -1;
if (!abs) in->seekg(pos, ios_base::cur);
else if (pos>=0) in->seekg(pos, ios_base::beg);
else in->seekg(pos, ios_base::end);
return tell();
}
bool operator! () { return (in && !*in) || (out && !*out); }
};

static void ioWrite (QS::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
if (io.encoder) {
istringstream in(f.getString(1));
io.encoder(in, *io.out);
}
else {
int length;
const char* buffer = f.getBuffer<char>(1, &length);
io.out->write(buffer, length);
}}

static void ioIterate (QS::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
if (io.in && *io.in) f.setBool(0, true);
else f.setNull(0);
}

static void ioRead (QS::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
int num = f.getOptionalNum(1, -1);
ostringstream out;
if (!io.in || !*io.in) { f.setNull(0); return; }
if (io.decoder) {
if (num<0) io.decoder(*io.in, out, 0);
else while(--num>=0 && io.in && *io.in) io.decoder(*io.in, out, 1);
f.setString(0, out.str());
}
else if (num<0) {
out << io.in->rdbuf();
string s = out.str();
f.setBuffer(0, s.data(), s.size());
}
else {
auto buf = make_unique<char[]>(num);
io.in->read(&buf[0], num);
f.setBuffer(0, &buf[0], io.in->gcount());
}}

static void ioReadLine (QS::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
if (!io.in || !*io.in) { f.setNull(0); return; }
if (io.decoder) {
ostringstream out;
io.decoder(*io.in, out, 2);
f.setString(0, out.str());
}
else {
string s;
getline(*io.in, s);
f.setBuffer(0, s.data(), s.size());
}}

static IO ioOpen (const string& target, optional<string> omode, optional<string> encoding) {
string mode = omode.value_or("r");
string enc = encoding.value_or("utf8");
if (mode.size()>3) { enc=mode; mode=""; }
bool reading = mode.find("r")!=string::npos;
bool writing = mode.find("w")!=string::npos;
bool appending = mode.find("a")!=string::npos;
bool binary = mode.find("b")!=string::npos;
if (reading) {
if (binary) IO(make_shared<ifstream>(target, ios::binary));
else return IO(make_shared<ifstream>(target), QS::VM::getDecoder(enc));
}
else if (writing) {
if (binary) IO(make_shared<ofstream>(target, ios::binary));
else return IO(make_shared<ofstream>(target), QS::VM::getEncoder(enc));
}
else if (appending) {
if (binary) IO(make_shared<ofstream>(target, ios::binary | ios::app));
else return IO(make_shared<ofstream>(target, ios::app), QS::VM::getEncoder(enc));
}
throw std::logic_error(format("Unknown open mode: %s", mode));
}

static IO ioCreate (optional<string> encoding) {
if (encoding) return IO(make_shared<ostringstream>(), QS::VM::getEncoder(encoding.value_or("utf8")));
else return IO(make_shared<ostringstream>());
}

static void ioOf (QS::Fiber& f) {
string enc = f.getOptionalString(2, "");
if (f.isBuffer(1)) {
int length;
const void* buf = f.getBufferV(1, &length);
string s(reinterpret_cast<const char*>(buf), length);
auto st = make_shared<istringstream>(s);
if (enc.empty()) f.emplaceUserObject<IO>(0, st);
else f.emplaceUserObject<IO>(0, st, QS::VM::getDecoder(enc));
}
else if (f.isString(1)) {
string s = f.getString(1);
auto st = make_shared<istringstream>(s);
if (enc.empty()) f.emplaceUserObject<IO>(0, st);
else f.emplaceUserObject<IO>(0, st, QS::VM::getDecoder(enc));
}
else throw std::logic_error("Expecting a buffer or a string");
}

static void ioToBuffer (QS::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
auto out = dynamic_pointer_cast<ostringstream>(io.out);
if (!out) throw std::logic_error("Couldn't convert to buffer");
string s = out->str();
f.setBuffer(0, s.data(), s.size());
}

void registerIO (QS::Fiber& f) {
f.loadGlobal("Sequence");
f.registerClass<IO>("IO", 1);
f.registerDestructor<IO>();
f.registerStaticMethod("open", STATIC_METHOD(ioOpen));
f.registerStaticMethod("create", STATIC_METHOD(ioCreate));
f.registerStaticMethod("of", ioOf);
f.registerMethod("read", ioRead);
f.registerMethod("readLine", ioReadLine);
f.registerMethod("!", METHOD(IO, operator!));
f.registerMethod("iterate", ioIterate);
f.registerMethod("iteratorValue", ioReadLine);
f.registerMethod("write", ioWrite);
f.registerMethod("flush", METHOD(IO, flush));
f.registerMethod("seek", METHOD(IO, seek));
f.registerMethod("tell", METHOD(IO, tell));
f.registerMethod("close", METHOD(IO, close));
f.registerMethod("toBuffer", ioToBuffer);
f.pop();
}

