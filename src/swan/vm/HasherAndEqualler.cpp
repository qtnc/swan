#include "HasherAndEqualler.hpp"
#include "FiberVM.hpp"

size_t hashBytes (const uint8_t* c, const uint8_t* end) {
size_t re = FNV_OFFSET;
for (; c<end; ++c) re = (re^*c) * FNV_PRIME;
return re;
}



size_t QVHasher::operator() (const QV& value) const {
QFiber& f = vm.getActiveFiber();
static int hashCodeSymbol = f.vm.findMethodSymbol("hashCode");
f.pushCppCallFrame();
f.push(value);
f.callSymbol(hashCodeSymbol, 1);
size_t re = f.getNum(-1);
f.pop();
f.popCppCallFrame();
return re;
}

bool QVEqualler::operator() (const QV& a, const QV& b) const {
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

