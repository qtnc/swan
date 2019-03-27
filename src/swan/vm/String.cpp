#include "String.hpp"
#include "FiberVM.hpp"
using namespace std;

#define MAX_CACHED_STRING_LENGTH 16

void appendToString (QFiber& f, QV x, string& out) {
if (x.isString()) {
QString& s = *x.asObject<QString>();
out.insert(out.end(), s.begin(), s.end());
}
else {
f.pushCppCallFrame();
f.push(x);
f.callSymbol(f.vm.findMethodSymbol("toString"), 1);
QString& s = *f.at(-1).asObject<QString>();
out.insert(out.end(), s.begin(), s.end());
f.pop();
f.popCppCallFrame();
}}

QV::QV (QVM& vm, const std::string& s): QV(QString::create(vm, s), QV_TAG_STRING) {}
QV::QV (QVM& vm, const char* s, int length): QV(QString::create(vm, s, length), QV_TAG_STRING) {}

std::string QV::asString () const { return asObject<QString>()->asString(); }
const char* QV::asCString () const { return asObject<QString>()->begin(); }
QString::QString (QVM& vm, size_t len): 
QSequence(vm.stringClass), length(len) {}

QString* QString::create (QVM& vm, const std::string& str) {
return QString::create(vm, str.data(), str.length());
}

QString* QString::create (QVM& vm, const char* str, int len) {
if (len<0) len = strlen(str);
if (len<=MAX_CACHED_STRING_LENGTH) {
auto it = vm.stringCache.find(make_pair(str, str+len));
if (it!=vm.stringCache.end()) return it->second;
}
QString* s = vm.constructVLS<QString, char>(len+1, vm, len);
if (len>0) memcpy(s->data, str, len+1);
s->data[len] = 0;
if (len<=MAX_CACHED_STRING_LENGTH) vm.stringCache.insert(make_pair(make_pair(s->begin(), s->end()), s));
return s;
}

QString* QString::create (QString* s) { 
return create(s->type->vm, s->data, s->length); 
}

QString::~QString () {
if (length<=MAX_CACHED_STRING_LENGTH)  type->vm.stringCache.erase(make_pair(begin(), end()));
}

QStringIterator::QStringIterator (QVM& vm, QString& m): 
QObject(vm.stringIteratorClass), str(m), 
iterator(m.begin())
{}

