#include "Class.hpp"
#include "Value.hpp"
#include "ForeignClass.hpp"
#include "VM.hpp"
#include "ExtraAlgorithms.hpp"

QClass::QClass (QVM& vm0, QClass* type0, QClass* parent0, const std::string& name0, int nf):
QObject(type0), 
vm(vm0), 
parent(parent0), nFields(nf), name(name0),
methods(trace_allocator<QV>(vm))
{ copyParentMethods(); }

QClass* QClass::create (QVM& vm, QClass* type, QClass* parent, const std::string& name, int nStaticFields, int nFields) { 
return vm.constructVLS<QClass, QV>(nStaticFields, vm, type, parent, name, nFields); 
}

QClass* QClass::copyParentMethods () {
if (parent && parent->methods.size()) methods = parent->methods;
else methods.clear();
return this;
}

QClass* QClass::mergeMixinMethods (QClass* cls) {
insert_n(methods, static_cast<int>(cls->methods.size())-static_cast<int>(methods.size()), QV::UNDEFINED);
for (int i=0; i<cls->methods.size(); i++) if (!cls->methods[i].isNullOrUndefined()) methods[i] = cls->methods[i];
return this;
}

QClass* QClass::bind (const std::string& methodName, QNativeFunction func) {
int symbol = vm.findMethodSymbol(methodName);
return bind(symbol, QV(func));
}

QClass* QClass::bind (int symbol, const QV& val) {
insert_n(methods, 1+symbol-methods.size(), QV::UNDEFINED);
methods[symbol] = val;
return this;
}

QObject* QClass::instantiate () {
return QInstance::create(this, nFields);
}


QClass& QV::getClass (QVM& vm) {
if (isUndefined()) return *vm.undefinedClass;
else if (isBool()) return *vm.boolClass;
else if (isNum()) return *vm.numClass;
else if (isString()) return *vm.stringClass;
else if (isNull()) return *vm.nullClass;
else if (isNativeFunction())  return *vm.functionClass;
else if (isGenericSymbolFunction())  return *vm.functionClass;
else {
QClass* cls = asObject<QObject>()->type;
return cls? *cls : *vm.classClass;
}}


QForeignClass::QForeignClass (QVM& vm0, QClass* type0, QClass* parent0, const std::string& name0, int nf, DestructorFn destr):
QClass(vm0, type0, parent0, name0, nf), 
destructor(destr)
{}


QObject* QForeignClass::instantiate () {
return QForeignInstance::create(this, nFields);
}


