#include "Fiber.hpp"
#include "Vm.hpp"
#include "Fiber_inlines.hpp"
#include "BoundFunction.hpp"
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
}

void QFiber::callSymbol (int symbol, int nArgs) {
QV receiver = *(stack.end() -nArgs);
QClass& cls = receiver.getClass(vm);
QV method = cls.findMethod(symbol);
bool re = callMethod(method, nArgs);
if (!re) runtimeError("%s has no method %s", cls.name, vm.methodSymbols[symbol]);
}

void QFiber::callSuperSymbol (int symbol, int nArgs) {
uint32_t newStackBase = stack.size() -nArgs;
QV receiver = stack.at(newStackBase);
QClass* cls = receiver.getClass(vm) .parent;
QV method = cls->findMethod(symbol);
bool re = callMethod(method, nArgs);
if (!re) {
runtimeError("%s has no method %s", cls->name, vm.methodSymbols[symbol]);
}}

bool QFiber::callMethod (QV& method, int nArgs) {
uint32_t newStackBase = stack.size() -nArgs;
QV receiver = stack.at(newStackBase);
if (method.isNativeFunction()) {
QNativeFunction func = method.asNativeFunction();
callFrames.push_back({nullptr, nullptr, newStackBase});
func(*this);
stack.resize(newStackBase+1);
callFrames.pop_back();
return true;
}
else if (method.isClosure()) {
QClosure& closure = *method.asObject<QClosure>();
callClosure(closure, nArgs);
return true;
}
else {
stack.resize(newStackBase);
push(QV());
return false;
}}

void QFiber::callMethod (const string& name, int nArgs) {
int symbol = vm.findMethodSymbol(name);
pushCppCallFrame();
callSymbol(symbol, nArgs);
popCppCallFrame();
}

void QFiber::callCallable (int nArgs) {
uint32_t newStackBase = stack.size() -nArgs;
QV& method = stack.at(newStackBase -1);
const QClass& cls = method.getClass(vm);
if (method.isNativeFunction()) {
QNativeFunction func = method.asNativeFunction();
callFrames.push_back({nullptr, nullptr, newStackBase});
func(*this);
stack.at(newStackBase -1) = stack.at(newStackBase);
stack.resize(newStackBase);
callFrames.pop_back();
}
else if (method.isClosure()) {
QClosure* closure = method.asObject<QClosure>();
stack.erase(stack.begin() + newStackBase -1);
callClosure(*closure, nArgs);
}
else if (method.isFiber()) {
QFiber& f = *method.asObject<QFiber>();
callFiber(f, nArgs);
stack.erase(stack.begin() + newStackBase -1);
}
else if (method.isBoundFunction()) {
BoundFunction& bf = *method.asObject<BoundFunction>();
stack.insert(stack.begin() + newStackBase, bf.object);
method = bf.method;
callCallable(nArgs+1);
}
else if (method.isGenericSymbolFunction()) {
uint_method_symbol_t symbol = method.asInt<uint_method_symbol_t>();
stack.erase(stack.end() -nArgs -1);
callSymbol(symbol, nArgs);
}
else if (method.isNull()) {
stack.resize(newStackBase -1);
push(QV());
runtimeError("%s isn't callable", method.getClass(vm).name);
}
else {
int symbol = vm.findMethodSymbol(("()"));
QClass& cls = method.getClass(vm);
callSymbol(symbol, nArgs+1);
}}

void QFiber::adjustArguments (int& nArgs, int nClosureArgs, bool vararg) {
if (nArgs>=nClosureArgs) {
if (vararg) {
QTuple* tuple = QTuple::create(vm, nArgs+1-nClosureArgs, &stack.at(stack.size() +nClosureArgs -nArgs -1));
stack.erase(stack.end() +nClosureArgs -nArgs -1, stack.end());
push(tuple);
}
else if (nArgs>nClosureArgs) stack.erase(stack.end() +nClosureArgs -nArgs, stack.end());
nArgs = nClosureArgs;
}
else {
while (nArgs<nClosureArgs) {
push(QV());
nArgs++;
}
if (vararg) {
pop();
push(QTuple::create(vm, 0, nullptr));
}}
}

void QFiber::callClosure (QClosure& closure, int nArgs) {
adjustArguments(nArgs, closure.func.nArgs, closure.func.vararg);
uint32_t newStackBase = stack.size() -nArgs;
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
f.parentFiber = curFiber;
curFiber = &f;
f.run();
curFiber = this;
}break;
case FiberState::YIELDED:
if (nArgs>=1) {
f.push(top());
stack.erase(stack.end() -nArgs, stack.end());
}
else f.push(QV());
f.parentFiber = curFiber;
curFiber = &f;
f.run();
curFiber = this;
break;
case FiberState::RUNNING:
case FiberState::FINISHED:
case FiberState::FAILED:
default:
runtimeError(("Couldn't call a running or finished fiber"));
push(QV());
return;
}
push(f.top());
f.pop();
}

Upvalue* QFiber::captureUpvalue (int slot) {
QV val = QV(QV_TAG_OPEN_UPVALUE | reinterpret_cast<uintptr_t>(&stack.at(callFrames.back().stackBase + slot)));
QFiber* _this=this;
auto it = find_if(openUpvalues.begin(), openUpvalues.end(), [&](auto x){ return x->fiber==_this && x->value.i==val.i; });
if (it!=openUpvalues.end()) return *it;
auto upvalue = new Upvalue(*this, slot);
openUpvalues.push_back(upvalue);
return upvalue;
}
