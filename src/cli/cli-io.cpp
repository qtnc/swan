#include "../include/QScript.hpp"
#include "../include/QScriptBinding.hpp"
#include "../include/cpprintf.hpp"
#include<iostream>
#include<sstream>
#include<fstream>
#include<exception>
#include<memory>
using namespace std;

struct TextInput {
shared_ptr<istream> in;
TextInput (const shared_ptr<istream>& i): in(i) {}
string readLine () {
string line;
getline(*in, line);
return line;
}
string readAll () {
ostringstream out;
out << in->rdbuf();
return out.str();
}
string iteratorValue (int unused) { return readLine(); }
optional<int> iterate () { optional<int> o; if (*in) o=1; return o; }
void close () {}
};

struct TextOutput {
shared_ptr<ostream> out;
TextOutput (const shared_ptr<ostream>& o): out(o) {}
void close () {}
void write (const std::string& s) { 
(*out) << s; 
}
};

struct TextFileOutput: TextOutput {
TextFileOutput (const std::string& filename): TextOutput(make_shared<ofstream>(filename)) {}
void close () { 
out->flush(); 
static_pointer_cast<ofstream>(out)->close(); 
}
};

struct TextFileInput: TextInput {
TextFileInput (const std::string& filename): TextInput(make_shared<ifstream>(filename)) {}
void close () { 
static_pointer_cast<ifstream>(in)->close(); 
}
};

struct TextMemOutput: TextOutput {
TextMemOutput (): TextOutput(make_shared<ostringstream>()) {}
string toString () { return static_pointer_cast<ostringstream>(out)->str(); }
};

struct TextMemInput: TextInput {
TextMemInput (const string& content): TextInput(make_shared<istringstream>(content)) {}
};

struct BinInput: TextInput {
BinInput (const shared_ptr<istream>& i): TextInput(i) {}
};

struct BinOutput: TextOutput  {
BinOutput (const shared_ptr<ostream>& o): TextOutput(o) {}
};

struct BinFileOutput: BinOutput {
BinFileOutput (const std::string& filename): BinOutput(make_shared<ofstream>(filename, ios::binary)) {}
void close () { 
out->flush(); 
static_pointer_cast<ofstream>(out)->close(); 
}
};

struct BinFileInput: BinInput {
BinFileInput (const std::string& filename): BinInput(make_shared<ifstream>(filename, ios::binary)) {}
void close () { 
static_pointer_cast<ifstream>(in)->close(); 
}
};

struct BinMemOutput: BinOutput {
BinMemOutput (): BinOutput(make_shared<ostringstream>()) {}
string toString () { return static_pointer_cast<ostringstream>(out)->str(); }
};

struct BinMemInput: BinInput {
BinMemInput (const void* data, int length): BinInput(make_shared<istringstream>(string(reinterpret_cast<const char*>(data), length))) {}
};

static void binReadLine (QS::Fiber& f) {
string s = f.getUserObject<BinInput>(0) .readLine();
f.setBuffer(0, s.data(), s.size());
}

static void binReadAll (QS::Fiber& f) {
string s = f.getUserObject<BinInput>(0).readAll();
f.setBuffer(0, s.data(), s.size());
}

static void binWrite (QS::Fiber& f) {
auto& b = f.getUserObject<BinOutput>(0);
int length;
const void* buf = f.getBufferV(1, &length);
b.out->write(reinterpret_cast<const char*>(buf), length);
}

static void binMemInputConstructor (QS::Fiber& f) {
void* ptr = f.getUserPointer(0);
int length;
const void* data = f.getBufferV(1, &length);
new(ptr) BinMemInput(data, length);
}

static void binMemOutputToBuffer (QS::Fiber& f) {
string s = f.getUserObject<BinMemOutput>(0).toString();
f.setBuffer(0, s.data(), s.length());
}

void registerIO (QS::Fiber& f) {
f.loadGlobal("Sequence");
f.registerClass<TextInput>("Reader", 1);
f.registerMethod("readLine", METHOD(TextInput, readLine));
f.registerMethod("read", METHOD(TextInput, readAll));
f.registerMethod("iterate", METHOD(TextInput, iterate));
f.registerMethod("iteratorValue", METHOD(TextInput, iteratorValue));
f.registerMethod("close", METHOD(TextInput, close));
f.pushCopy();
f.registerClass<TextFileInput>("FileReader", 1);
f.registerConstructor<TextFileInput, const std::string&>();
f.registerDestructor<TextFileInput>();
f.registerMethod("close", METHOD(TextFileInput, close));
f.pop();
f.pushCopy();
f.registerClass<TextMemInput>("StringReader", 1);
f.registerConstructor<TextMemInput, const std::string&>();
f.registerDestructor<TextMemInput>();
f.pop();
f.pop();

f.registerClass<TextOutput>("Writer");
f.registerMethod("write", METHOD(TextOutput, write));
f.registerMethod("close", METHOD(TextOutput, close));
f.pushCopy();
f.registerClass<TextFileOutput>("FileWriter", 1);
f.registerConstructor<TextFileOutput, const std::string&>();
f.registerDestructor<TextFileOutput>();
f.registerMethod("close", METHOD(TextFileOutput, close));
f.pop();
f.pushCopy();
f.registerClass<TextMemOutput>("StringWriter", 1);
f.registerConstructor<TextMemOutput>();
f.registerDestructor<TextMemOutput>();
f.registerMethod("toString", METHOD(TextMemOutput, toString));
f.pop();
f.pop();

f.loadGlobal("Sequence");
f.registerClass<BinInput>("Input", 1);
f.registerMethod("readLine", binReadLine);
f.registerMethod("read", binReadAll);
f.registerMethod("iterate", METHOD(TextInput, iterate));
f.registerMethod("iteratorValue", binReadLine);
f.registerMethod("close", METHOD(TextInput, close));
f.pushCopy();
f.registerClass<BinFileInput>("FileInput", 1);
f.registerConstructor<BinFileInput, const std::string&>();
f.registerDestructor<BinFileInput>();
f.registerMethod("close", METHOD(BinFileInput, close));
f.pop();
f.pushCopy();
f.registerClass<BinMemInput>("BufferInput", 1);
f.registerMethod("constructor", binMemInputConstructor);
f.registerDestructor<BinMemInput>();
f.pop();
f.pop();

f.registerClass<BinOutput>("Output");
f.registerMethod("write", binWrite);
f.registerMethod("close", METHOD(TextOutput, close));
f.pushCopy();
f.registerClass<BinFileOutput>("FileOutput", 1);
f.registerConstructor<BinFileOutput, const std::string&>();
f.registerDestructor<BinFileOutput>();
f.registerMethod("close", METHOD(BinFileOutput, close));
f.pop();
f.pushCopy();
f.registerClass<BinMemOutput>("BufferOutput", 1);
f.registerConstructor<BinMemOutput>();
f.registerDestructor<BinMemOutput>();
f.registerMethod("toBuffer", binMemOutputToBuffer);
f.pop();
f.pop();
}

