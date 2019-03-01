#include "Fiber.hpp"
#include "VM.hpp"
#include "BoundFunction.hpp"
#include "Map.hpp"
#include "ExtraAlgorithms.hpp"
#include "OpCodeInfo.hpp"
#include "Fiber_inlines.hpp"
#include<string>
using namespace std;

static void instantiate (QFiber& f) {
int ctorSymbol = f.vm.findMethodSymbol("constructor");
QClass& cls = f.getObject<QClass>(0);
if (ctorSymbol>=cls.methods.size() || cls.methods[ctorSymbol].isNull()) {
f.runtimeError(("This class isn't instantiable"));
return;
}
QObject* instance = cls.instantiate();
f.setObject(0, instance);
f.callSymbol(ctorSymbol, f.getArgCount());
f.returnValue(instance);
}

void QVM::bindGlobal (const string& name, const QV& value) {
int symbol = findGlobalSymbol(name, true);
insert_n(globalVariables, 1+symbol-globalVariables.size(), QV());
globalVariables.at(symbol) = value;
}

QClass* QVM::createNewClass (const string& name, vector<QV>& parents, int nStaticFields, int nFields, bool foreign, QV& cval) {
LOCK_SCOPE(globalMutex)
string metaName = name + ("MetaClass");
QClass* parent = parents[0].asObject<QClass>();
QClass* meta = QClass::create(*this, classClass, classClass, metaName, 0, nStaticFields);
cval = meta;
QClass* cls = foreign?
new QForeignClass(*this, meta, parent, name, nFields):
QClass::create(*this, meta, parent, name, nStaticFields, nFields+std::max(0, parent->nFields));
cval = cls;
for (auto& p: parents) cls->mergeMixinMethods( p.asObject<QClass>() );
meta->bind("()", instantiate);
return cls;
}


void QFiber::pushNewClass (int nParents, int nStaticFields, int nFields) {
string name = at(-nParents -1).asString();
vector<QV> parents(stack.end() -nParents, stack.end());
stack.erase(stack.end() -nParents -1, stack.end());
QClass* parent = parents[0].asObject<QClass>();
if (parent->nFields<0) {
runtimeError("Unable to inherit from built-in class %s", parent->name);
return;
}
if (dynamic_cast<QForeignClass*>(parent)) {
runtimeError("Unable to inherit from foreign class %s", parent->name);
return;
}
pushNull();
QClass* cls = vm.createNewClass(name, parents, nStaticFields, nFields, false, at(-1));
}

void QFiber::pushNewForeignClass (const std::string& name, size_t id, int nUserBytes, int nParents) {
if (nParents<=0) {
push(vm.objectClass);
nParents = 1;
}
vector<QV> parents(stack.end() -nParents, stack.end());
pushNull();
QForeignClass* cls = static_cast<QForeignClass*>(vm.createNewClass(name, parents, 0, nUserBytes, true, at(-1)));
cls->id = id;
stack.erase(stack.end() -nParents, stack.end());
vm.foreignClassIds[id] = cls;
}

QV getItemFromLastArgMap (QFiber& f, const string& key) {
if (f.at(-1).isInstanceOf(f.vm.mapClass)) {
QMap& map = f.getObject<QMap>(-1);
return map.get(QV(f.vm, key));
}
return QV();
}

double QFiber::getOptionalNum (int stackIndex, const std::string& key, double defaultValue) {
QV value = getItemFromLastArgMap(*this, key);
if (value.isNum()) return value.asNum();
else return getOptionalNum(stackIndex, defaultValue);
}

bool QFiber::getOptionalBool (int stackIndex, const std::string& key, bool defaultValue) {
QV value = getItemFromLastArgMap(*this, key);
if (value.isBool()) return value.asBool();
else return getOptionalBool(stackIndex, defaultValue);
}

string QFiber::getOptionalString (int stackIndex, const std::string& key, const std::string& defaultValue) {
QV value = getItemFromLastArgMap(*this, key);
if (value.isString()) return value.asString();
else return getOptionalString(stackIndex, defaultValue);
}

Swan::Handle QFiber::getOptionalHandle  (int stackIndex, const std::string& key, const Swan::Handle& defaultValue) {
QV value = getItemFromLastArgMap(*this, key);
if (!value.isNull()) return value.asHandle();
else return getOptionalHandle(stackIndex, defaultValue);
}

void QFiber::loadGlobal (const string& name) {
int symbol = vm.findGlobalSymbol(name, false);
if (symbol<0) pushNull();
else push(vm.globalVariables.at(symbol));
}

void QFiber::storeGlobal (const string& name) {
vm.bindGlobal(name, top());
}

void QFiber::storeMethod (const string& name) {
storeMethod(vm.findMethodSymbol(name));
}

void QFiber::storeStaticMethod (const string& name) {
storeStaticMethod(vm.findMethodSymbol(name));
}

void QFiber::storeDestructor ( void(*destr)(void*) ) {
stack.back().asObject<QForeignClass>() ->destructor = destr;
}

void adjustFieldOffset (QFunction& func, int offset) {
for (const char *bc = func.bytecode.data(), *end = func.bytecode.data()+func.bytecode.length(); bc<end; ) {
uint8_t op = *bc++;
switch(op) {
case OP_LOAD_FIELD:
case OP_STORE_FIELD:
*reinterpret_cast<uint_field_index_t*>(const_cast<char*>(bc)) += offset;
break;
}
bc += OPCODE_INFO[op].nArgs;
}
}

void* QFiber::pushNewUserPointer (size_t id) {
QForeignClass& cls = *vm.foreignClassIds[id];
QForeignInstance* instance = static_cast<QForeignInstance*>(cls.instantiate());
push(instance);
return &instance->userData[0];
}

void* QFiber::setNewUserPointer (int idx, size_t id) {
QForeignClass& cls = *vm.foreignClassIds[id];
QForeignInstance* instance = static_cast<QForeignInstance*>(cls.instantiate());
at(idx) = instance;
return &instance->userData[0];
}

void QFiber::storeImport (const string& name) {
LOCK_SCOPE(vm.globalMutex)
vm.imports[name] = at(-1);
}

void QFiber::import (const std::string& baseFile, const std::string& requestedFile) {
LOCK_SCOPE(vm.globalMutex)
string finalFile = vm.pathResolver(baseFile, requestedFile);
auto it = vm.imports.find(finalFile);
if (it!=vm.imports.end()) push(it->second);
else {
vm.imports[finalFile] = true;
try {
loadFile(finalFile);
call(0);
vm.imports[finalFile] = at(-1);
} catch (...) { vm.imports.erase(finalFile); throw; }
}}
