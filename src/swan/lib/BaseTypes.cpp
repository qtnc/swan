#include "SwanLib.hpp"
#include "../vm/Fiber_inlines.hpp"
#include "../vm/BoundFunction.hpp"
#include "../../include/cpprintf.hpp"
using namespace std;

static void boolNot (QFiber& f) {
f.returnValue(!f.getBool(0)); 
}

static void boolToString (QFiber& f) {
f.returnValue(QV(f.vm, f.getBool(0)? "true" : "false")); 
}

static void boolInstantiate (QFiber& f) {
f.returnValue(!f.at(1).isFalsy()); 
}

static void undefinedToString (QFiber& f) {
f.returnValue(QV(f.vm, "undefined"));
}

static void nullToString (QFiber& f) {
f.returnValue(QV(f.vm, "null"));
}

static void nullEquals (QFiber& f) {
f.returnValue(f.isNullOrUndefined(1)); 
}

static void nullNotEquals (QFiber& f) {
f.returnValue(!f.isNullOrUndefined(1)); 
}

static void returnTrue (QFiber& f) {
f.returnValue(true);
}

static void returnFalse (QFiber& f) {
f.returnValue(false);
}

static void fiberInstantiate (QFiber& f) {
if (f.getArgCount()==1) f.returnValue(QV(&f, QV_TAG_FIBER));
else if (f.at(1).isClosure()) {
QClosure& closure = f.getObject<QClosure>(1);
QFiber* fb = f.vm.construct<QFiber>(f.vm, closure);
f.returnValue(QV(fb, QV_TAG_FIBER));
}
else f.returnValue(QV::UNDEFINED);
}

static void fiberNext (QFiber& f) {
f.callFiber(f.getObject<QFiber>(0), f.getArgCount() -1); 
f.returnValue(f.at(1)); 
}

static void objectInstantiate (QFiber& f) {
int ctorSymbol = f.vm.findMethodSymbol("constructor");
QClass& cls = f.getObject<QClass>(0);
if (ctorSymbol>=cls.methods.size() || cls.methods[ctorSymbol].isNullOrUndefined()) {
error<invalid_argument>("%s can't be instantiated, it has no method 'constructor'", cls.name);
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
QVHasher hash(f.vm);
auto h = hash(f.at(0));
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
f.returnValue(cls3.isSubclassOf(&cls1) || cls2.isSubclassOf(&cls1));
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

static void objectGetClass (QFiber& f) {
f.returnValue(&f.at(0).getClass(f.vm)); 
}

static void objclassName (QFiber& f) {
auto& cls = f.getObject<QClass>(0);
f.returnValue(QV(f.vm, cls.name.c_str())); 
}

static void functionInstantiate (QFiber& f) {
f.returnValue(QV::UNDEFINED);
if (f.isString(1)) {
QV adctx = f.getArgCount()>=3? f.at(2) : QV::UNDEFINED;
f.loadString(f.getString(1), "<eval>", "<eval>", adctx);
f.returnValue(f.at(-1));
}
else if (f.isNum(1)) {
int index = f.getNum(1) -1;
if (index<0 && -index<=f.callFrames.size()) {
auto cf = f.callFrames.end() +index;
if (cf->closure) f.returnValue(QV(cf->closure, QV_TAG_CLOSURE));
}}
}

static void functionBind (QFiber& f) {
f.returnValue(QV(BoundFunction::create(f.vm, f.at(0), f.getArgCount() -1, &f.at(1)), QV_TAG_BOUND_FUNCTION));
}

static string getFuncName (QV f, QVM& vm) {
if (f.isClosure()) return f.asObject<QClosure>()->func.name.str();
else if (f.isNormalFunction()) return f.asObject<QFunction>()->name.str();
else if (f.isGenericSymbolFunction()) return "::" + vm.methodSymbols[f.asInt<uint_method_symbol_t>()];
else if (f.isNativeFunction() || f.isStdFunction()) return "<native>";
else if (f.isFiber()) return getFuncName(QV(f.asObject<QFiber>() ->callFrames[0] .closure, QV_TAG_CLOSURE), vm) + ":<fiber>";
else if (f.isBoundFunction()) {
BoundFunction& bf = *f.asObject<BoundFunction>();
return format("bound(%d):%s", bf.count, getFuncName(bf.method, vm));
}
else return "<unknown>";
}

static void functionName (QFiber& f) {
f.returnValue(getFuncName(f.at(0), f.vm)); 
}

void QVM::initBaseTypes () {
objectClass
->copyParentMethods()
->bind("constructor", doNothing)
->bind("class", objectGetClass)
->bind("toString", objectToString, "OS")
->bind("hashCode", objectHashCode, "ON")
->bind("is", objectEquals, "OOB")
->bind("==", objectEquals, "OOB")
->bind("!=", objectNotEquals, "OOB")
->bind(findMethodSymbol("compare"), QV::UNDEFINED)
->bind("<", objectLess, "OOB")
->bind(">", objectGreater, "OOB")
->bind("<=", objectLessEquals, "OOB")
->bind(">=", objectGreaterEquals, "OOB")
->bind("!", returnFalse, "OB")
->assoc<QObject>();

classClass
->copyParentMethods()
->bind("()", objectInstantiate)
->bind("is", instanceofOperator, "OOB")
->bind("toString", objclassName, "OS")
->bind("name", objclassName, "OS")
->assoc<QClass>();

functionClass
->copyParentMethods()
->bind("bind", functionBind)
->bind("name", functionName)
->assoc<QFunction>();

boolClass
->copyParentMethods()
->bind("!", boolNot)
->bind("toString", boolToString)
;

nullClass
->copyParentMethods()
->bind("!", returnTrue)
->bind("toString", nullToString)
->bind("==", nullEquals)
->bind("!=", nullNotEquals)
;

undefinedClass
->copyParentMethods()
->bind("!", returnTrue)
->bind("toString", undefinedToString)
->bind("==", nullEquals)
->bind("!=", nullEquals)
;

iterableClass
->copyParentMethods()
->bind("iterator", doNothing)
->assoc<QObject>();

fiberClass
->copyParentMethods()
->bind("()", fiberNext)
->bind("next", fiberNext)
->bind("iterator", doNothing)
->assoc<QFiber>();

classClass->type
->copyParentMethods()
->assoc<QClass>();

objectClass->type
->copyParentMethods()
->assoc<QClass>();

boolClass ->type
->copyParentMethods()
->bind("()", boolInstantiate)
->assoc<QClass>();

functionClass ->type
->copyParentMethods()
->bind("()", functionInstantiate)
->assoc<QClass>();

fiberClass ->type
->copyParentMethods()
->bind("()", fiberInstantiate)
->assoc<QClass>();

iterableClass ->type
->copyParentMethods()
->assoc<QClass>();

iteratorClass ->type
->copyParentMethods()
->assoc<QClass>();
}
