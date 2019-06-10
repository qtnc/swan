#include "SwanLib.hpp"
#include "../vm/Fiber_inlines.hpp"
#include "../../include/cpprintf.hpp"
using namespace std;


static void fiberInstantiate (QFiber& f) {
QClosure& closure = f.getObject<QClosure>(1);
QFiber* fb = f.vm.construct<QFiber>(f.vm, closure);
f.returnValue(QV(fb, QV_TAG_FIBER));
}

static void fiberNext (QFiber& f) {
f.callFiber(f.getObject<QFiber>(0), f.getArgCount() -1); 
f.returnValue(f.at(1)); 
}

static void objectInstantiate (QFiber& f) {
int ctorSymbol = f.vm.findMethodSymbol("constructor");
QClass& cls = f.getObject<QClass>(0);
if (ctorSymbol>=cls.methods.size() || cls.methods[ctorSymbol].isNullOrUndefined()) {
f.runtimeError("%s has no constructor", cls.name);
return;
}
QObject* instance = cls.instantiate();
f.setObject(0, instance);
f.pushCppCallFrame();
f.callSymbol(ctorSymbol, f.getArgCount());
f.popCppCallFrame();
f.returnValue(instance);
}

void objectHashCode (QFiber& f) {
uint32_t h = FNV_OFFSET, *u = reinterpret_cast<uint32_t*>(&(f.at(0).i));
h ^= u[0] ^u[1];
f.returnValue(static_cast<double>(h));
}

static void objectToString (QFiber& f) {
const QClass& cls = f.at(0).getClass(f.vm);
f.returnValue(format("%s@%#0$16llX", cls.name, f.at(0).i) );
}

static void instanceofOperator (QFiber& f) {
QClass& cls1 = f.getObject<QClass>(0);
QClass& cls2 = f.at(1).getClass(f.vm);
if (cls2.isSubclassOf(f.vm.classClass)) {
QClass& cls3 = f.getObject<QClass>(1);
f.returnValue(cls3.isSubclassOf(&cls1));
}
else f.returnValue(cls2.isSubclassOf(&cls1));
}

static void objectEquals (QFiber& f) {
f.returnValue(f.at(0).i == f.at(1).i);
}

static void objectNotEquals (QFiber& f) {
// Call == so that users don't have do define != explicitly to implement full equality comparisons
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x1);
f.push(x2);
f.callSymbol(f.vm.findMethodSymbol("=="), 2);
bool re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
f.returnValue(!re);
}

static void objectLess (QFiber& f) {
// Implement < in termes of compare so that users only have to implement three way comparison
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x1);
f.push(x2);
f.callSymbol(f.vm.findMethodSymbol("compare"), 2);
double re = f.at(-1).asNum();
f.pop();
f.popCppCallFrame();
f.returnValue(re<0);
}


static void objectGreater (QFiber& f) {
// Implement >, >= and <= in termes of < so that users only have to implement < for full comparison
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x2);
f.push(x1);
f.callSymbol(f.vm.findMethodSymbol("<"), 2);
bool re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
f.returnValue(re);
}

static void objectLessEquals (QFiber& f) {
// Implement >, >= and <= in termes of < so that users only have to implement < for full comparison
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x2);
f.push(x1);
f.callSymbol(f.vm.findMethodSymbol("<"), 2);
bool re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
f.returnValue(!re);
}

static void objectGreaterEquals (QFiber& f) {
// Implement >, >= and <= in termes of < so that users only have to implement < for full comparison
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x1);
f.push(x2);
f.callSymbol(f.vm.findMethodSymbol("<"), 2);
bool re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
f.returnValue(!re);
}

static void functionInstantiate (QFiber& f) {
f.returnValue(QV::UNDEFINED);
if (f.isString(1)) {
int symbol = f.vm.findMethodSymbol(f.getString(1));
f.returnValue(QV(symbol | QV_TAG_GENERIC_SYMBOL_FUNCTION));
}
else if (f.isNum(1)) {
int index = f.getNum(1) -1;
if (index<0 && -index<=f.callFrames.size()) {
auto cf = f.callFrames.end() +index;
if (cf->closure) f.returnValue(QV(cf->closure, QV_TAG_CLOSURE));
}}
}

void QVM::initBaseTypes () {
objectClass
->copyParentMethods()
BIND_L(class, { f.returnValue(&f.at(0).getClass(f.vm)); })
BIND_F(toString, objectToString)
BIND_F(is, objectEquals)
BIND_F(==, objectEquals)
BIND_F(!=, objectNotEquals)
->bind(findMethodSymbol("compare"), QV::UNDEFINED)
BIND_F(<, objectLess)
BIND_F(>, objectGreater)
BIND_F(<=, objectLessEquals)
BIND_F(>=, objectGreaterEquals)
BIND_L(!, { f.returnValue(QV(false)); })
BIND_L(?, { f.returnValue(QV(true)); })
;

classClass
->copyParentMethods()
BIND_F( (), objectInstantiate)
BIND_F(is, instanceofOperator)
BIND_L(toString, { f.returnValue(QV(f.vm, f.getObject<QClass>(0) .name)); })
BIND_F(hashCode, objectHashCode)
BIND_L(name, { f.returnValue(QV(f.vm, f.getObject<QClass>(0) .name)); })
;

functionClass
->copyParentMethods()
BIND_L( (), {  f.callMethod(f.at(0), f.getArgCount() -1);  f.returnValue(f.at(1));  })
BIND_F(hashCode, objectHashCode)
;

boolClass
->copyParentMethods()
BIND_L(!, { f.returnValue(!f.getBool(0)); })
BIND_N(?)
BIND_L(toString, { f.returnValue(QV(f.vm, f.getBool(0)? "true" : "false")); })
BIND_F(hashCode, objectHashCode)
;

nullClass
->copyParentMethods()
BIND_L(!, { f.returnValue(QV::TRUE); })
BIND_L(?, { f.returnValue(QV::FALSE); })
BIND_L(toString, {
if (f.isNull(0)) f.returnValue(QV(f.vm, "null", 4)); 
else f.returnValue(QV(f.vm, "undefined", 9)); 
})
BIND_L(==, { f.returnValue(f.isNullOrUndefined(1)); })
BIND_F(hashCode, objectHashCode)
;

iterableClass
->copyParentMethods()
BIND_N(iterator)
;

fiberClass
->copyParentMethods()
BIND_F( (), fiberNext)
BIND_F(next, fiberNext)
BIND_N(iterator)
;

classClass->type
->copyParentMethods()
;

objectClass->type
->copyParentMethods()
;

boolClass ->type
->copyParentMethods()
BIND_L( (), { f.returnValue(!f.at(1).isFalsy()); })
;

functionClass ->type
->copyParentMethods()
BIND_F( (), functionInstantiate)
;

fiberClass ->type
->copyParentMethods()
BIND_F( (), fiberInstantiate)
;

iterableClass ->type
->copyParentMethods()
;

iteratorClass ->type
->copyParentMethods()
;
}
