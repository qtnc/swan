#include "SwanLib.hpp"
using namespace std;

void stringFormat (QFiber& f);

static void import_  (QFiber& f) {
string curFile = f.getString(0), requestedFile = f.ensureString(1)->asString();
f.import(curFile, requestedFile);
f.returnValue(f.at(-1));
}

static void loadGlobal (QFiber& f) {
f.loadGlobal(f.getString(0));
f.returnValue(f.at(-1));
}

static void storeGlobal (QFiber& f) {
f.storeGlobal(f.getString(0));
f.returnValue(f.at(-1));
}

static void storeMethod (QFiber& f) {
f.pushCopy(0);
f.pushCopy(2);
f.storeMethod(f.getString(1));
f.returnValue(f.at(-1));
}

static void storeStaticMethod (QFiber& f) {
f.pushCopy(0);
f.pushCopy(2);
f.storeStaticMethod(f.getString(1));
f.returnValue(f.at(-1));
}

static void loadMethod (QFiber& f) {
f.returnValue(f.loadMethod(f.at(0), f.vm.findMethodSymbol(f.getString(1))));
}

static void loadField (QFiber& f) {
int index = f.getNum(1);
f.returnValue(f.at(0).asObject<QInstance>() ->fields[index]);
}

static void loadStaticField (QFiber& f) {
int index = f.getNum(1);
f.returnValue( f.at(0).getClass(f.vm) .staticFields[index] );
}

static void storeField (QFiber& f) {
int index = f.getNum(1);
f.at(0).asObject<QInstance>() ->fields[index] = f.at(-1);
f.returnValue(f.at(-1));
}

static void storeStaticField (QFiber& f) {
int index = f.getNum(1);
f.at(0).getClass(f.vm) .staticFields[index]  = f.at(-1);
f.returnValue(f.at(-1));
}

static void createClass (QFiber& f) {
string name = f.getString(0);
vector<QV> parents;
int nFields=-1, nStaticFields=-1;
for (int i=1, n=f.getArgCount(); i<n; i++) {
if (f.isNum(i)) {
int x = f.getNum(i);
if (nFields<0) nFields=x;
else nStaticFields=x;
}
else if (f.isString(i)) {
//todo
}
else parents.push_back(f.at(i));
}
if (nFields<0) nFields=0;
if (nStaticFields<0) nStaticFields=0;
if (!parents.size()) parents.push_back(f.vm.objectClass);
QClass* cls = f.vm.createNewClass(name, parents, nStaticFields, nFields, false, f.at(0));
f.returnValue(cls);
}

void QVM::initGlobals () {
systemMetaClass ->copyParentMethods()
BIND_L(gc, { f.vm.garbageCollect(); f.returnValue(QV()); })
;
systemClass->copyParentMethods();

QClass* globalClasses[] = { 
boolClass, classClass, fiberClass, functionClass, listClass, mapClass, nullClass, numClass, objectClass, rangeClass, sequenceClass, setClass, stringClass, systemClass, tupleClass
#ifndef NO_BUFFER
, bufferClass
#endif
#ifndef NO_REGEX
, regexClass
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
, dictionaryClass, linkedListClass, priorityQueueClass, sortedSetClass
#endif
#ifndef NO_GRID
, gridClass
#endif
#ifndef NO_RANDOM
, randomClass
#endif
};
for (auto cls: globalClasses) bindGlobal(cls->name, cls);

bindGlobal("import", import_);
bindGlobal("format", stringFormat);

#ifndef NO_REFLECT
bindGlobal("loadMethod", loadMethod);
bindGlobal("storeMethod", storeMethod);
bindGlobal("storeStaticMethod", storeStaticMethod);
bindGlobal("loadGlobal", loadGlobal);
bindGlobal("storeGlobal", storeGlobal);
bindGlobal("loadField", loadField);
bindGlobal("storeField", storeField);
bindGlobal("createClass", createClass);
#endif
}
