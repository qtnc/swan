#include "Core.hpp"
#include "Upvalue.hpp"
#include "Object.hpp"
#include "BoundFunction.hpp"
#include "StdFunction.hpp"
#include "Function.hpp"
#include "Closure.hpp"
#include "ForeignInstance.hpp"
#include "ForeignClass.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"

Upvalue::Upvalue (QFiber& f, int slot): 
QObject(f.vm.objectClass), fiber(&f), value(QV(static_cast<uint64_t>(QV_TAG_OPEN_UPVALUE | reinterpret_cast<uintptr_t>(&f.stack.at(stackpos(f, slot)))))) 
{}

Upvalue::Upvalue (QFiber& f, const QV& v): 
QObject(f.vm.objectClass), fiber(&f), value(v) 
{}

QObject::QObject (QClass* tp):
type(tp), next(nullptr) {
if (type && &type->vm) type->vm.addToGC(this);
}

void* QObject::gcOrigin () {
return static_cast<QObject*>(this); 
}

QFunction::QFunction (QVM& vm): 
QObject(vm.functionClass), 
nArgs(0), vararg(false),
constants(trace_allocator<QV>(vm)),
upvalues(trace_allocator<Upvalue>(vm))
{}

QClosure::QClosure (QVM& vm, QFunction& f):
QObject(vm.functionClass), func(f) {}

BoundFunction::BoundFunction (QVM& vm, const QV& m, size_t c):
QObject(vm.functionClass), method(m), count(c) {}

BoundFunction* BoundFunction::create (QVM& vm, const QV& m, size_t c, const QV* a) {
auto bf = vm.constructVLS<BoundFunction, QV>(c, vm, m, c);
memcpy(bf->args, a, c*sizeof(QV));
return bf;
}


StdFunction::StdFunction (QVM& vm, const StdFunction::Func& func0):
QObject(vm.functionClass), func(func0) {}

QInstance* QInstance::create (QClass* type, int nFields) { 
return type->vm.constructVLS<QInstance, QV>(nFields, type); 
}

size_t QInstance::getMemSize () { 
return sizeof(*this) + sizeof(QV) * std::max(0, type->nFields); 
}

QForeignInstance* QForeignInstance::create (QClass* type, int nBytes) { 
return type->vm.constructVLS<QForeignInstance, char>(nBytes, type); 
}

size_t QForeignInstance::getMemSize () { 
return sizeof(*this) + std::max(0, type->nFields); 
}

QForeignInstance::~QForeignInstance () {
QForeignClass* cls = static_cast<QForeignClass*>(type);
if (cls->destructor) cls->destructor(userData);
}

size_t QClosure::getMemSize () { 
return sizeof(*this) + sizeof(QV) * func.upvalues.size(); 
}

void QVM::addToGC (QObject* obj) {
if (gcMemUsage >=gcTreshhold && !gcLock) garbageCollect();
obj->gcNext(firstGCObject);
firstGCObject  = obj;
}
