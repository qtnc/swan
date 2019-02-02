#include "SwanLib.hpp"
using namespace std;

void stringFormat (QFiber& f);

static void import_  (QFiber& f) {
string curFile = f.getString(0), requestedFile = f.getString(1);
f.import(curFile, requestedFile);
f.returnValue(f.at(-1));
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
, dictionaryClass, linkedListClass
#endif
#ifndef NO_RANDOM
, randomClass
#endif
};
for (auto cls: globalClasses) bindGlobal(cls->name, cls);

bindGlobal("import", import_);
bindGlobal("format", stringFormat);
}
