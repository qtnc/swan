#include "VLS.hpp"
#include "Core.hpp"
#include "Upvalue.hpp"
#include "Object.hpp"
#include "BoundFunction.hpp"
#include "Function.hpp"
#include "Closure.hpp"
#include "ForeignInstance.hpp"
#include "ForeignClass.hpp"
#include "VM.hpp"

Upvalue::Upvalue (QFiber& f, int slot): 
QObject(f.vm.objectClass), fiber(&f), value(QV(static_cast<uint64_t>(QV_TAG_OPEN_UPVALUE | reinterpret_cast<uintptr_t>(&f.stack.at(stackpos(f, slot)))))) 
{}

QObject::QObject (QClass* tp):
type(tp), next(nullptr) {
if (type && &type->vm) type->vm.addToGC(this);
}

QFunction::QFunction (QVM& vm): QObject(vm.functionClass), nArgs(0), vararg(false)  {}

QClosure::QClosure (QVM& vm, QFunction& f):
QObject(vm.functionClass), func(f) {}

BoundFunction::BoundFunction (QVM& vm, const QV& o, const QV& m):
QObject(vm.functionClass), object(o), method(m) {}




QForeignInstance::~QForeignInstance () {
QForeignClass* cls = static_cast<QForeignClass*>(type);
if (cls->destructor) cls->destructor(userData);
}

void QVM::addToGC (QObject* obj) {
LOCK_SCOPE(globalMutex)
if (gcAliveCount++ >= gcTreshhold) {
garbageCollect();
}
obj->next = firstGCObject;
firstGCObject = obj;
}

