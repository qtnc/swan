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
QS::VM::EncodingConversionFn codec;
unique_ptr<istream> in;
unique_ptr<ostream> out;
IO (unique_ptr<istream> i, const QS::VM::EncodingConversionFn& c = nullptr): in(std::move(i)), out(nullptr), codec(c) {}
IO (unique_ptr<ostream> o, const QS::VM::EncodingConversionFn& c = nullptr): in(nullptr), out(std::move(o)), codec(c) {}
};

static void ioWrite (QS::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
if (io.codec) {
istringstream in(f.getString(1));
io.codec(in, *io.out);
}
else {
int length;
const char* buffer = f.getBuffer<char>(1, &length);
io.out->write(buffer, length);
}}

static void ioRead (QS::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
ostringstream out;
if (io.codec) {
io.codec(*io.in, out);
f.setString(0, out.str());
}
else {
out << io.in->rdbuf();
string s = out.str();
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
if (binary) IO(std::move(make_unique<ifstream>(target, ios::binary)));
else return IO(std::move(make_unique<ifstream>(target)), QS::VM::getDecoder(enc));
}
else if (writing) {
if (binary) IO(make_unique<ofstream>(target, ios::binary));
else return IO(make_unique<ofstream>(target), QS::VM::getEncoder(enc));
}
else if (appending) {
if (binary) IO(make_unique<ofstream>(target, ios::binary | ios::app));
else return IO(make_unique<ofstream>(target, ios::app), QS::VM::getEncoder(enc));
}
throw std::logic_error(format("Unknown open mode: %s", mode));
}

void registerIO (QS::Fiber& f) {
f.loadGlobal("Sequence");
f.registerClass<IO>("IO", 1);
f.registerDestructor<IO>();
f.registerStaticMethod("open", STATIC_METHOD(ioOpen));
f.registerMethod("read", ioRead);
f.registerMethod("write", ioWrite);
f.pop();
}

