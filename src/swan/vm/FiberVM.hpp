#ifndef _____FIBER_IMPL_HPP_____
#define _____FIBER_IMPL_HPP_____
#include "Fiber.hpp"
#include "VM.hpp"
#include "ForeignClass.hpp"

inline Swan::VM& QFiber::getVM () { return vm; }

inline void QFiber::setString  (int i, const std::string& s)  { at(i) = QV(vm,s); }
inline void QFiber::pushString (const std::string& s) { stack.push_back(QV(vm,s)); }
inline void QFiber::setCString  (int i, const char* s)  { at(i) = QV(vm,s); }
inline void QFiber::pushCString (const char* s) { stack.push_back(QV(vm,s)); }
inline QString* QFiber::ensureString (QV& val) {
if (val.isString()) return val.asObject<QString>();
else {
int toStringSymbol = vm.findMethodSymbol("toString");
pushCppCallFrame();
push(val);
callSymbol(toStringSymbol, 1);
QString* re = at(-1).asObject<QString>();
pop();
popCppCallFrame();
return re;
}}

inline bool QFiber::isUserPointer (int i, size_t classId) {
return at(i) .isInstanceOf( vm.foreignClassIds[classId] );
}

template<class F> static inline void iterateSequence (QFiber& f, const QV& initial, const F& func) {
int iteratorSymbol = f.vm.findMethodSymbol(("iterator"));
int iterateSymbol = f.vm.findMethodSymbol(("iterate"));
int iteratorValueSymbol = f.vm.findMethodSymbol(("iteratorValue"));
f.push(initial);
f.pushCppCallFrame();
f.callSymbol(iteratorSymbol, 1);
f.popCppCallFrame();
QV iterable = f.at(-1), key, value;
f.pop();
while(true){
f.push(iterable);
f.push(key);
f.pushCppCallFrame();
f.callSymbol(iterateSymbol, 2);
f.popCppCallFrame();
key = f.at(-1);
f.pop();
if (key.isNull()) break;
f.push(iterable);
f.push(key);
f.pushCppCallFrame();
f.callSymbol(iteratorValueSymbol, 2);
f.popCppCallFrame();
value = f.at(-1);
f.pop();
func(value);
}}

#endif
