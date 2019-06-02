#ifndef NO_BUFFER
#include "SwanLib.hpp"
#include "../vm/Buffer.hpp"
#include "../../include/cpprintf.hpp"
#include<fstream>
#include<cstdlib>
#include<boost/algorithm/string.hpp>
#include<utf8.h>
using namespace std;

static void bufferIterator (QFiber& f) {
QBuffer& b = f.getObject<QBuffer>(0);
int index = f.getOptionalNum(1, 0);
if (index<0) index += b.length +1;
auto it = f.vm.construct<QBufferIterator>(f.vm, b);
if (index>0) std::advance(it->iterator, index);
f.returnValue(it);
}

static void bufferIteratorNext (QFiber& f) {
QBufferIterator& li = f.getObject<QBufferIterator>(0);
if (li.iterator < li.buf.end()) f.returnValue(static_cast<double>(*li.iterator++));
else f.returnValue(QV::UNDEFINED);
}

static void bufferIteratorPrevious (QFiber& f) {
QBufferIterator& li = f.getObject<QBufferIterator>(0);
if (li.iterator > li.buf.begin()) f.returnValue(static_cast<double>(*--li.iterator));
else f.returnValue(QV::UNDEFINED);
}


static void bufferHashCode (QFiber& f) {
QBuffer& b = f.getObject<QBuffer>(0);
size_t re = hashBytes(b.begin(), b.end());
f.returnValue(static_cast<double>(re));
}

static void bufferPlus (QFiber& f) {
QBuffer &first = f.getObject<QBuffer>(0), &second = f.getObject<QBuffer>(1);
uint32_t length = first.length + second.length;
QBuffer* result = f.vm.constructVLS<QBuffer, uint8_t>(length+1, f.vm, length);
memcpy(result->data, first.data, first.length);
memcpy(result->data + first.length, second.data, second.length);
result->data[length] = 0;
f.returnValue(result);
}

static void bufferFromSequence (QFiber& f) {
vector<QV, trace_allocator<QV>> values(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
if (f.isNum(i) || f.isBuffer(i)) values.push_back(f.at(i));
else f.getObject<QSequence>(i) .copyInto(f, values);
}
string re;
auto out = back_inserter(re);
for (auto& val: values) {
if (val.isInstanceOf(f.vm.bufferClass)) {
QBuffer& b = *val.asObject<QBuffer>();
re.insert(re.end(), reinterpret_cast<const char*>(b.begin()), reinterpret_cast<const char*>(b.end()));
}
else if (val.isNum()) *out++ = static_cast<char>(val.asNum());
}
QBuffer* b = QBuffer::create(f.vm, reinterpret_cast<const uint8_t*>(re.data()), re.size());
f.returnValue(b);
}

static void bufferSubscript (QFiber& f) {
QBuffer& b = f.getObject<QBuffer>(0);
if (f.isNum(1)) {
int pos = f.getNum(1);
if (pos<0) pos+=b.length;
f.returnValue(pos<0||pos>=b.length? QV::UNDEFINED : QV(static_cast<double>(b.data[pos])));
}
else if (f.isRange(1)) {
int start=0, end=0;
f.getRange(1).makeBounds(b.length, start, end);
f.returnValue(QBuffer::create(f.vm, &b.data[start], &b.data[end]));
}}

static void bufferFind (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
int start = f.getOptionalNum(2, 0);
if (start<0) start += b.length;
auto endPos = b.end(), startPos = b.begin()+start;
auto re = search(startPos, endPos, needle.begin(), needle.end());
if (re==endPos) f.returnValue(-1);
else f.returnValue(static_cast<double>(re - b.begin()));
}

static void bufferRfind (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
int end = f.getOptionalNum(2, b.length);
if (end<0) end += b.length;
auto startPos = b.begin(), endPos = b.begin()+end;
auto re = find_end(startPos, endPos, needle.begin(), needle.end());
if (re==endPos) f.returnValue(-1);
else f.returnValue(static_cast<double>(re - b.begin()));
}

static void bufferFindFirstOf (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
int start = f.getOptionalNum(2, 0);
if (start<0) start += b.length;
auto endPos = b.end(), startPos = b.begin()+start;
auto re = find_first_of(startPos, endPos, needle.begin(), needle.end());
if (re==endPos) f.returnValue(-1);
else f.returnValue(static_cast<double>(re - b.begin()));
}

static void bufferIn (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
auto it = search(b.begin(), b.end(), needle.begin(), needle.end());
f.returnValue(it!=b.end());
}

static void bufferStartsWith (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
if (needle.length>b.length) { f.returnValue(QV(false)); return; }
else f.returnValue(equal(needle.begin(), needle.end(), b.begin()));
}

static void bufferEndsWith (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
if (needle.length>b.length) { f.returnValue(QV(false)); return; }
else f.returnValue(equal(needle.begin(), needle.end(), b.end() - needle.length));
}

static string normalizeEncodingName (const string& name) {
string enc = boost::to_lower_copy(name);
auto it = remove_if(enc.begin(), enc.end(), boost::is_any_of("-_"));
enc.erase(it, enc.end());
return enc;
}

Swan::VM::EncodingConversionFn export Swan::VM::getEncoder (const std::string& name) {
return QVM::stringToBufferConverters[normalizeEncodingName(name)];
}

Swan::VM::DecodingConversionFn export Swan::VM::getDecoder (const std::string& name) {
return QVM::bufferToStringConverters[normalizeEncodingName(name)];
}

void export Swan::VM::registerEncoder (const std::string& name, const Swan::VM::EncodingConversionFn& func) {
QVM::stringToBufferConverters[normalizeEncodingName(name)] = func;
}

void export Swan::VM::registerDecoder (const std::string& name, const Swan::VM::DecodingConversionFn& func) {
QVM::bufferToStringConverters[normalizeEncodingName(name)] = func;
}

static QString* convertBufferToString (QBuffer& b, const string& encoding) {
auto it = QVM::bufferToStringConverters.find(normalizeEncodingName(encoding));
if (it==QVM::bufferToStringConverters.end()) throw std::logic_error(format("No converter found to convert from %s to %s", encoding, "UTF-8"));
istringstream in(string(reinterpret_cast<const char*>(b.begin()), reinterpret_cast<const char*>(b.end())));
ostringstream out;
(it->second)(in, out, 0);
return QString::create(b.type->vm, out.str());
}

static QBuffer* convertStringToBuffer (QString& s, const string& encoding) {
auto it = QVM::stringToBufferConverters.find(normalizeEncodingName(encoding));
if (it==QVM::stringToBufferConverters.end()) throw std::logic_error(format("No converter found to convert from %s to %s", "UTF-8", encoding));
istringstream in(string(s.begin(), s.end()));
ostringstream out;
(it->second)(in, out);
string re = out.str();
return QBuffer::create(s.type->vm, &re[0], re.size());
}

static void bufferInstantiate (QFiber& f) {
QString* s = f.ensureString(1);
string enc = f.getOptionalString(2, "UTF-8");
f.returnValue(convertStringToBuffer(*s, enc));
}

static void stringInstantiate (QFiber& f) {
if (f.isBuffer(1)) {
QBuffer& b = f.getObject<QBuffer>(1);
QString* enc = f.ensureString(2);
f.returnValue(convertBufferToString(b, enc->asString()));
}
else f.returnValue(QV( f.ensureString(1), QV_TAG_STRING));
}

static void stringToBuffer (QFiber& f) {
QString &s = f.getObject<QString>(0), *enc = f.ensureString(1);
f.returnValue(convertStringToBuffer(s, enc->asString()));
}

static void bufferToString (QFiber& f) {
QBuffer& b = f.getObject<QBuffer>(0);
if (f.getArgCount()>=2) {
QString* enc = f.ensureString(1);
f.returnValue(convertBufferToString(b, enc->asString()));
} 
else {
string out = "Buffer:";
for (uint8_t x: b) {
switch(x){
case 0: out += "\\0"; break;
case '\b': out += "\\b"; break;
case 0x1B: out += "\\e"; break;
case '\f': out += "\\f"; break;
case '\n': out += "\\n"; break;
case '\r': out += "\\r"; break;
case '\t': out += "\\t"; break;
default: 
if (x<32 || x>=127) out += format("\\x%0$2X", static_cast<int>(x));
else out += static_cast<char>(x);
break;
}}
f.returnValue(QV(QString::create(f.vm, out), QV_TAG_STRING));
}}

static void stringFromSequence (QFiber& f) {
vector<QV, trace_allocator<QV>> values(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
if (f.isNum(i) || f.isString(i)) values.push_back(f.at(i));
else f.getObject<QSequence>(i) .copyInto(f, values);
}
string re;
auto out = back_inserter(re);
for (auto& val: values) {
if (val.isString()) {
QString& s = *val.asObject<QString>();
re.insert(re.end(), s.begin(), s.end());
}
else if (val.isNum()) utf8::append(val.asNum(), out);
else {
QString& s = *f.ensureString(val);
re.insert(re.end(), s.begin(), s.end());
}}
f.returnValue(re);
}

void QVM::initBufferType () {
stringClass BIND_F(toBuffer, stringToBuffer);

bufferClass
->copyParentMethods()
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QBuffer>(0).length)); })
BIND_F([], bufferSubscript)
BIND_F(+, bufferPlus)
BIND_F(in, bufferIn)
BIND_F(iterator, bufferIterator)
BIND_F(indexOf, bufferFind)
BIND_F(lastIndexOf, bufferRfind)
BIND_F(findFirstOf, bufferFindFirstOf)
BIND_F(startsWith, bufferStartsWith)
BIND_F(endsWith, bufferEndsWith)
BIND_F(toString, bufferToString)
;

bufferIteratorClass
->copyParentMethods()
BIND_F(next, bufferIteratorNext)
BIND_F(previous, bufferIteratorPrevious)
;


stringClass ->type
->copyParentMethods()
BIND_F( (), stringInstantiate)
BIND_F( of, stringFromSequence)
;

bufferClass ->type
->copyParentMethods()
BIND_F( (), bufferInstantiate)
BIND_F(of, bufferFromSequence)
;
}
#endif
