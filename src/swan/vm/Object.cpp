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

inline void QVM::addToGC (QObject* obj) {
#ifdef DEBUG_GC
if (!gcLock) garbageCollect();
#endif
if (gcMemUsage >=gcTreshhold && !gcLock) garbageCollect();
obj->gcNext(firstGCObject);
firstGCObject  = obj;
}

