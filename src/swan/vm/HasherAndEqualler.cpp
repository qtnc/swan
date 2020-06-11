#include "HasherAndEqualler.hpp"
#include "FiberVM.hpp"
#include "NatSort.hpp"

bool stringEquals (QString& s1, QString& s2);

// Adapated from MurmurHash3, x86, 32
// https://github.com/aappleby/smhasher
template<class T> inline T rot (T x, uint8_t r) {
return (x<<r) | (x>>((sizeof(T)<<3)-r));
}
size_t hashBytes (const uint8_t* begin, const uint8_t* end) {
const uint32_t C1 = 0xcc9e2d51;
const uint32_t C2 = 0x1b873593;
const int nBlocks = (end-begin)/4;
const uint32_t* p = reinterpret_cast<const uint32_t*>(begin), *pEnd = p+nBlocks;
uint32_t h = 0x811c9dc5;
while(p<pEnd) {
uint32_t k = *p++;
k *= C1;
k = rot(k, 15);
k *= C2;
h ^= k;
h = rot(h, 13);
h = h*5+0xe6546b64;
}
uint32_t k = 0;
switch(reinterpret_cast<uintptr_t>(end) &3) {
case 3: k ^= end[-3] <<16; [[fallthrough]];
case 2: k ^= end[-2] << 8; [[fallthrough]];
case 1: k ^= end[-1];
k*= C1;
k = rot(k, 15);
k *= C2;
h ^= k;
}
h ^= (end-begin);
h ^= (h>>16);
h *= 0x85ebca6b;
h ^= (h>>13);
h *= 0xc2b2ae35;
h ^= (h>>16);
return h;
}


size_t QVHasher::operator() (const QV& value) const {
if (value.isString()) {
QString& s = *value.asObject<QString>();
return hashBytes(reinterpret_cast<const uint8_t*>(s.data), reinterpret_cast<const uint8_t*>(s.data+s.length));
}
else if (value.isNum()) {
return value.i ^(value.i>>32ULL) ^(value.i>>53ULL);
}
else if (!value.isObject()) {
return value.i ^(value.i>>32ULL);
}
else {
QFiber& f = vm.getActiveFiber();
static int hashCodeSymbol = f.vm.findMethodSymbol("hashCode");
f.pushCppCallFrame();
f.push(value);
f.callSymbol(hashCodeSymbol, 1);
size_t re = f.getNum(-1);
f.pop();
f.popCppCallFrame();
return re;
}}

bool QVEqualler::operator() (const QV& a, const QV& b) const {
if (a.isString() && b.isString()) return stringEquals(*a.asObject<QString>(), *b.asObject<QString>());
else if (a.isNum() && b.isNum()) return a.d==b.d;
QFiber& f = vm.getActiveFiber();
static int eqeqSymbol = f.vm.findMethodSymbol("==");
f.pushCppCallFrame();
f.push(a);
f.push(b);
f.callSymbol(eqeqSymbol, 2);
bool re = f.getBool(-1);
f.pop();
f.popCppCallFrame();
return re;
}

bool QVLess::operator() (const QV& a, const QV& b) const {
if (a.isString() && b.isString()) return strnatcmp(a.asObject<QString>()->data, b.asObject<QString>()->data) <0;
else if (a.isNum() && b.isNum()) return a.d<b.d;
QFiber& f = vm.getActiveFiber();
static int lessSymbol = f.vm.findMethodSymbol("<");
f.pushCppCallFrame();
f.push(a);
f.push(b);
f.callSymbol(lessSymbol, 2);
bool re = f.getBool(-1);
f.pop();
f.popCppCallFrame();
return re;
}

bool QVBinaryPredicate::operator() (const QV& a, const QV& b) const {
QFiber& f = vm.getActiveFiber();
f.pushCppCallFrame();
f.push(func);
f.push(a);
f.push(b);
f.call(2);
bool re = f.getBool(-1);
f.pop();
f.popCppCallFrame();
return re;
}

bool QVUnaryPredicate::operator() (const QV& a) const {
QFiber& f = vm.getActiveFiber();
f.pushCppCallFrame();
f.push(func);
f.push(a);
f.call(1);
bool re = f.getBool(-1);
f.pop();
f.popCppCallFrame();
return re;
}

