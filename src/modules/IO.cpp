#include "../include/Swan.hpp"
#include "../include/SwanBinding.hpp"
#include<fstream>
#include<sstream>
#include<memory>
#include<optional>
using namespace std;

struct Reader {
shared_ptr<istream> in;
bool isValid () { return in && !!*in; }
bool isNotValid () { return !isValid(); }
void close () {  in.reset(); }
optional<string> readLine () {
optional<string> result;
string str;
if (getline(*in, str)) result=str;
return result;
}
string readAll () {
ostringstream out;
out << in->rdbuf();
return out.str();
}
optional<string> readCount (int max) {
optional<string> result;
string str;
str.reserve(max+1);
bool read = !!in->read(const_cast<char*>(str.data()), max);
if (read) {
str.resize(in->gcount());
result = str;
}
return result;
}
optional<string> read (optional<int> max) {
if (max.has_value()) return readCount(max.value());
else return readAll();
}
long long tell () { return in->tellg(); }
long long seekrel (long long n) {
in->seekg(n, ios::cur);
return tell();
}
long long seekabs (long long n) {
if (n>=0) in->seekg(n, ios::beg);
else in->seekg(n, ios::end);
return tell();
}
};

struct Writer {
shared_ptr<ostream> out;
bool isValid () { return out && !!*out; }
bool isNotValid () { return !isValid(); }
void close () { out.reset(); }
void flush () { out->flush(); }
void write (const string& str) { out->write(str.data(), str.size()); }
};

struct FileReader: Reader {
FileReader (const string& file, ios::openmode mode) { in = make_shared<ifstream>(file, mode); }
};

struct FileWriter: Writer {
FileWriter (const string& file, ios::openmode mode) { out = make_shared<ofstream>(file, mode); }
};

struct StringReader: Reader {
StringReader (const string& str) { in = make_shared<istringstream>(str, ios::binary); }
};

struct StringWriter: Writer {
StringWriter () { out = make_shared<ostringstream>(ios::binary); }
string toString () { return dynamic_pointer_cast<ostringstream>(out)->str(); }
};

static void readerNext (Swan::Fiber& f) {
Reader& r = f.getUserObject<Reader>(0);
optional<string> result = r.readLine();
if (result.has_value()) f.setString(0, result.value());
else f.setUndefined(0);
}

static ios::openmode decodeMode (Swan::Fiber& f, int index, ios::openmode mode = ios::in ^ios::in) {
string modeStr = f.getOptionalString("mode", "");
if (f.getOptionalBool("binary", false)) mode |= ios::binary;
if (f.getOptionalBool("append", false)) mode |= ios::app;
for (char c: modeStr) {
switch(c){
case 'a': mode |= ios::app; break;
case 'b': mode |= ios::binary; break;
case 'r': mode |= ios::in; break;
case 't': mode &=~ios::binary; break;
case 'w': mode |= ios::out; break;
}}
return mode;
}

void fileReaderOpen (Swan::Fiber& f) {
string file = f.getString(1);
ios::openmode mode = decodeMode(f, 2, ios::in);
f.setEmplaceUserObject<FileReader>(0, file, mode);
}

void fileWriterOpen (Swan::Fiber& f) {
string file = f.getString(1);
ios::openmode mode = decodeMode(f, 2, ios::out);
f.setEmplaceUserObject<FileWriter>(0, file, mode);
}

void swanLoadIO (Swan::Fiber& f) {
f.pushNewMap();

f.loadGlobal("Iterable");
f.pushNewClass<Reader>("Reader", 1);
f.registerMethod("?", METHOD(Reader, isValid));
f.registerMethod("!", METHOD(Reader, isNotValid));
f.registerMethod("readLine", METHOD(Reader, readLine));
f.registerMethod("read", METHOD(Reader, read));
f.registerMethod("close", METHOD(Reader, close));
f.registerMethod("next", readerNext);
f.registerMethod("seek", METHOD(Reader, seekrel));
f.registerProperty("position", METHOD(Reader, tell), METHOD(Reader, seekabs));
f.registerDestructor<Reader>();
f.storeIndex(-2, "Reader");

f.pushCopy();
f.pushNewClass<FileReader>("FileReader", 1);
f.registerStaticMethod("()", fileReaderOpen);
f.registerDestructor<FileReader>();
f.storeIndex(-3, "FileReader");
f.pop(); // FileReader

f.pushCopy();
f.pushNewClass<StringReader>("StringReader", 1);
f.registerConstructor<StringReader, string>();
f.registerDestructor<StringReader>();
f.storeIndex(-3, "StringReader");
f.pop(); // StringReader

f.pop(); // Reader

f.pushNewClass<Writer>("Writer");
f.registerMethod("?", METHOD(Writer, isValid));
f.registerMethod("!", METHOD(Writer, isNotValid));
f.registerMethod("write", METHOD(Writer, write));
f.registerMethod("flush", METHOD(Writer, flush));
f.registerMethod("close", METHOD(Writer, close));
f.registerDestructor<Writer>();
f.storeIndex(-2, "Writer");

f.pushCopy();
f.pushNewClass<FileWriter>("FileWriter", 1);
f.registerStaticMethod("()", fileWriterOpen);
f.storeIndex(-3, "FileWriter");
f.pop(); // FileWriter

f.pushCopy();
f.pushNewClass<StringWriter>("StringWriter", 1);
f.registerMethod("toString", METHOD(StringWriter, toString));
f.registerConstructor<StringWriter>();
f.registerDestructor<StringWriter>();
f.storeIndex(-3, "StringWriter");
f.pop(); // StringWriter

f.pop(); // Writer
}

