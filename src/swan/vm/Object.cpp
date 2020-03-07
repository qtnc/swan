#include "Object.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"


QObject::QObject (QClass* tp):
type(tp), next(nullptr) {
if (type && &type->vm) type->vm.addToGC(this);
}

void* QObject::gcOrigin () {
return static_cast<QObject*>(this); 
}

void QVM::addToGC (QObject* obj) {
if (gcMemUsage >=gcTreshhold && !gcLock) garbageCollect();
//if (!gcLock) garbageCollect();
obj->gcNext(firstGCObject);
firstGCObject  = obj;
}
