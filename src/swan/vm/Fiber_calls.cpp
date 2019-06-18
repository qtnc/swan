#include "Fiber.hpp"
#include "Vm.hpp"
#include "Fiber_inlines.hpp"
#include "BoundFunction.hpp"
#include "StdFunction.hpp"
#include "OpCodeInfo.hpp"
#include "Tuple.hpp"
#include "Upvalue.hpp"
#include "../../include/cpprintf.hpp"
#include<string>
using namespace std;

void QFiber::call (int nArgs) {
pushCppCallFrame();
callCallable(nArgs);
popCppCallFrame();
if (state==FiberState::YIELDED) runtimeError("Yielding across C++ call frame");
}

void QFiber::callSymbol (int symbol, int nArgs) {
QV receiver = *(stack.end() -nArgs);
QClass& cls = receiver.getClass(vm);
QV method = cls.findMethod(symbol);
bool re = callFunc(method, nArgs);
if (!re) runtimeError("%s has no method %s", cls.name, vm.methodSymbols[symbol]);
}

void QFiber::callSuperSymbol (int symbol, int nArgs) {
uint32_t newStackBase = stack.size() -nArgs;
QV receiver = stack.at(newStackBase);
QClass* cls = receiver.getClass(vm) .parent;
QV method = cls->findMethod(symbol);
bool re = callFunc(method, nArgs);
if (!re) {
runtimeError("%s has no method %s", cls->name, vm.methodSymbols[symbol]);
}}

inline bool QFiber::callFunc (QV& method, int nArgs) {
uint32_t newStackBase = stack.size() -nArgs;
QV receiver = stack.at(newStackBase);

if (method.isClosure()) {
QClosure& closure = *method.asObject<QClosure>();
callClosure(closure, nArgs);
return true;
}
else if (method.isNativeFunction()) {
QNativeFunction func = method.asNativeFunction();
callFrames.push_back({nullptr, nullptr, newStackBase});
func(*this);
stack.resize(newStackBase+1);
callFrames.pop_back();
return true;
}
else if (method.isGenericSymbolFunction()) {
uint_method_symbol_t symbol = method.asInt<uint_method_symbol_t>();
callSymbol(symbol, nArgs);
return true;
}
else if (method.isBoundFunction()) {
BoundFunction& bf = *method.asObject<BoundFunction>();
stack.insert(stack.begin() + newStackBase +1, bf.args, bf.args+bf.count);
stack.insert(stack.begin() + newStackBase, method);
callCallable(nArgs+bf.count);
return true;
}
else if (method.isStdFunction()) {
const StdFunction::Func& func = method.asObject<StdFunction>()->func;
callFrames.push_back({nullptr, nullptr, newStackBase});
func(*this);
stack.resize(newStackBase+1);
callFrames.pop_back();
return true;
}
else {
stack.resize(newStackBase);
pushUndefined();
return false;
}
}

void QFiber::callMethod (const string& name, int nArgs) {
int symbol = vm.findMethodSymbol(name);
pushCppCallFrame();
callSymbol(symbol, nArgs);
popCppCallFrame();
if (state==FiberState::YIELDED) runtimeError("Yielding across C++ call frame");
}

void QFiber::callCallable (int nArgs) {
uint32_t newStackBase = stack.size() -nArgs;
QV& method = stack.at(newStackBase -1);
const QClass& cls = method.getClass(vm);

if (method.isClosure()) {
QClosure* closure = method.asObject<QClosure>();
stack.erase(stack.begin() + newStackBase -1);
callClosure(*closure, nArgs);
return;
}
else if (method.isNativeFunction()) {
auto native = method.asNativeFunction();
callFrames.push_back({nullptr, nullptr, newStackBase});
native(*this);
stack.at(newStackBase -1) = stack.at(newStackBase);
stack.resize(newStackBase);
callFrames.pop_back();
return;
}
else if (method.hasTag(QV_TAG_DATA)) {
int symbol = vm.findMethodSymbol(("()"));
QClass& cls = method.getClass(vm);
callSymbol(symbol, nArgs+1);
return;
}
else if (method.isFiber()) {
QFiber& f = *method.asObject<QFiber>();
callFiber(f, nArgs);
stack.erase(stack.begin() + newStackBase -1);
return;
}
else if (method.isGenericSymbolFunction()) {
uint_method_symbol_t symbol = method.asInt<uint_method_symbol_t>();
stack.erase(stack.end() -nArgs -1);
callSymbol(symbol, nArgs);
return;
}
else if (method.isBoundFunction()) {
BoundFunction& bf = *method.asObject<BoundFunction>();
stack.insert(stack.begin() + newStackBase, bf.args, bf.args+bf.count);
method = bf.method;
callCallable(nArgs+bf.count);
return;
}
else if (method.isStdFunction()) {
const StdFunction::Func& func = method.asObject<StdFunction>()->func;
callFrames.push_back({nullptr, nullptr, newStackBase});
func(*this);
stack.at(newStackBase -1) = stack.at(newStackBase);
stack.resize(newStackBase);
callFrames.pop_back();
return;
}
else {
return;
}
}

void QFiber::adjustArguments (int nArgs, int nClosureArgs, bool vararg) {
if (!vararg) {
if (nArgs==nClosureArgs) return;
else if (nArgs>nClosureArgs) stack.erase(stack.end() +nClosureArgs -nArgs, stack.end()); 
else while(nArgs++<nClosureArgs) pushUndefined();
}
else { // vararg
if (nArgs>=nClosureArgs) {
QTuple* tuple = QTuple::create(vm, nArgs+1-nClosureArgs, &stack.at(stack.size() +nClosureArgs -nArgs -1));
stack.erase(stack.end() +nClosureArgs -nArgs -1, stack.end());
push(tuple);
}
else {
while (++nArgs<nClosureArgs) pushUndefined();
push(QTuple::create(vm, 0, nullptr));
}}
}

inline void QFiber::callClosure (QClosure& closure, int nArgs) {
int nClosureArgs = closure.func.nArgs;
adjustArguments(nArgs, nClosureArgs, closure.func.vararg);
uint32_t newStackBase = stack.size() -nClosureArgs;
bool doRun = callFrames.back().isCppCallFrame();
callFrames.push_back({&closure, closure.func.bytecode.data(), newStackBase});
if (doRun) run();
}

void QFiber::callFiber (QFiber& f, int nArgs) {
switch(f.state){
case FiberState::INITIAL: {
QClosure& closure = *f.callFrames.back().closure;
f.stack.insert(f.stack.end(), stack.end() -nArgs, stack.end());
stack.erase(stack.end() -nArgs, stack.end());
f.adjustArguments(nArgs, closure.func.nArgs, closure.func.vararg);
f.parentFiber = this;
vm.activeFiber = &f;
f.run();
vm.activeFiber = this;
}break;
case FiberState::YIELDED:
if (nArgs==0) f.pushUndefined();
else {
f.push(top());
stack.erase(stack.end() -nArgs, stack.end());
}
f.parentFiber = this;
vm.activeFiber = &f;
f.run();
vm.activeFiber = this;
break;
case FiberState::RUNNING:
case FiberState::FAILED:
default:
runtimeError(("Couldn't call a running or finished fiber")); [[fallthrough]];
case FiberState::FINISHED:
pushUndefined();
return;
}
push(f.top());
f.pop();
}

Upvalue* QFiber::captureUpvalue (int slot) {
QV val = QV(QV_TAG_OPEN_UPVALUE | reinterpret_cast<uintptr_t>(&stack.at(callFrames.back().stackBase + slot)));
auto it = find_if(openUpvalues.begin(), openUpvalues.end(), [&](auto x){ return x->fiber==this && x->value.i==val.i; });
if (it!=openUpvalues.end()) return *it;
auto upvalue = vm.construct<Upvalue>(*this, slot);
openUpvalues.push_back(upvalue);
return upvalue;
}
