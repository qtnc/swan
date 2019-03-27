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
val = re;
return re;
}}

inline bool QFiber::isUserPointer (int i, size_t classId) {
return at(i) .isInstanceOf( vm.foreignClassIds[classId] );
}

template<class F> static inline void iterateSequence (QFiber& f, const QV& initial, const F& func) {
int iteratorSymbol = f.vm.findMethodSymbol(("iterator"));
int nextSymbol = f.vm.findMethodSymbol(("next"));
int hasNextSymbol = f.vm.findMethodSymbol(("hasNext"));
f.push(initial);
f.pushCppCallFrame();
f.callSymbol(iteratorSymbol, 1);
f.popCppCallFrame();
QV iterable = f.at(-1), key, value;
f.pop();
while(true){
f.push(iterable);
f.pushCppCallFrame();
f.callSymbol(hasNextSymbol, 1);
f.popCppCallFrame();
key = f.at(-1);
f.pop();
if (key.isFalsy()) break;
f.push(iterable);
f.pushCppCallFrame();
f.callSymbol(nextSymbol, 1);
f.popCppCallFrame();
value = f.at(-1);
f.pop();
func(value);
}}

#endif
