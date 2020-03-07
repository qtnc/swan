#include "Fiber.hpp"
#include "VM.hpp"
#include "BoundFunction.hpp"
#include "Map.hpp"
#include "Value.hpp"
#include "Iterable.hpp"
#include "ExtraAlgorithms.hpp"
#include "Array.hpp"
#include "OpCodeInfo.hpp"
#include "Fiber_inlines.hpp"
#include<string>
using namespace std;

static void instantiate (QFiber& f) {
int ctorSymbol = f.vm.findMethodSymbol("constructor");
QClass& cls = f.getObject<QClass>(0);
if (ctorSymbol>=cls.methods.size() || cls.methods[ctorSymbol].isNullOrUndefined()) {
error<invalid_argument>("%s can't be instantiated, it has no method 'constructor'", cls.name);
return;
}
QObject* instance = cls.instantiate();
f.setObject(0, instance);
f.callSymbol(ctorSymbol, f.getArgCount());
f.returnValue(instance);
}

void QVM::bindGlobal (const string& name, const QV& value, bool isConst) {
int symbol = findGlobalSymbol(name, 1 | (isConst?2:0));
insert_n(globalVariables, 1+symbol-globalVariables.size(), QV::UNDEFINED);
globalVariables.at(symbol) = value;
}

void QVM::bindGlobal (const string& name, QNativeFunction value, const char* typeInfo) {
bindGlobal(name, value, false);
nativeFuncTypeInfos[value] = typeInfo;
}


QClass* QVM::createNewClass (const string& name, vector<QV>& parents, int nStaticFields, int nFields, bool foreign) {
GCLocker gcLocker(*this);
string metaName = name + ("MetaClass");
QClass* parent = parents[0].asObject<QClass>();
QClass* meta = QClass::create(*this, classClass, classClass, metaName, 0, nStaticFields);
QClass* cls = foreign?
construct<QForeignClass>(*this, meta, parent, name, nFields):
QClass::create(*this, meta, parent, name, nStaticFields, nFields+parent->nFields);
if (foreign) cls->assoc<QForeignInstance>(true);
else cls->assoc<QInstance>(true);
meta->assoc<QClass>(true);
for (auto& p: parents) cls->mergeMixinMethods( p.asObject<QClass>() );
meta->bind("()", instantiate);
return cls;
}


void QFiber::pushNewClass (int nParents, int nStaticFields, int nFields) {
string name = at(-nParents -1).asString();
vector<QV> parents(stack.end() -nParents, stack.end());
stack.erase(stack.end() -nParents -1, stack.end());
QClass* parent = parents[0].asObject<QClass>();
auto it = find_if(parents.begin(), parents.end(), [&](auto& p){ return !p.isInstanceOf(vm.classClass); });
if (it!=parents.end()) {
error<invalid_argument>("Invalid parent class: %s", it->getClass(vm).name);
return;
}
if (parent->nonInheritable) {
error<invalid_argument>("Can't inherit from built-in class %s", parent->name);
return;
}
if (parent->foreign) {
error<invalid_argument>("Can't inherit from foreign class %s", parent->name);
return;
}
if (nFields+parent->nFields>=std::numeric_limits<uint_field_index_t>::max()) {
error<invalid_argument>("Too many member fields");
return;
}
GCLocker gcLocker(vm);
QClass* cls = vm.createNewClass(name, parents, nStaticFields, nFields, false);
push(cls);
}

void QFiber::pushNewForeignClass (const std::string& name, size_t id, int nUserBytes, int nParents) {
if (nParents<=0) {
push(vm.objectClass);
nParents = 1;
}
GCLocker gcLocker(vm);
vector<QV> parents(stack.end() -nParents, stack.end());
QForeignClass* cls = static_cast<QForeignClass*>(vm.createNewClass(name, parents, 0, nUserBytes, true));
cls->id = id;
cls->foreign = true;
stack.erase(stack.end() -nParents, stack.end());
vm.foreignClassIds[id] = cls;
push(cls);
}

QV getItemFromLastArgMap (QFiber& f, const string& key) {
if (f.at(-1).isInstanceOf(f.vm.mapClass)) {
QMap& map = f.getObject<QMap>(-1);
return map.get(QV(f.vm, key));
}
return QV::UNDEFINED;
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

void* QFiber::getOptionalUserPointer (int stackIndex, const std::string& key, size_t classId, void* defaultValue) {
QV value = getItemFromLastArgMap(*this, key);
if (value.isInstanceOf( vm.foreignClassIds[classId] )) return value.asObject<QForeignInstance>()->userData;
else return getOptionalUserPointer(stackIndex, classId, defaultValue);
}

Swan::Handle QFiber::getOptionalHandle  (int stackIndex, const std::string& key, const Swan::Handle& defaultValue) {
QV value = getItemFromLastArgMap(*this, key);
if (!value.isNullOrUndefined()) return value.asHandle();
else return getOptionalHandle(stackIndex, defaultValue);
}

vector<double> QFiber::getNumList (int stackIndex) {
vector<QV, trace_allocator<QV>> v0(vm);
auto citr = copyVisitor(std::back_inserter(v0));
at(stackIndex).copyInto(*this, citr);
vector<double> v;
for (QV x: v0) v.push_back(x.d);
return v;
}

vector<string> QFiber::getStringList (int stackIndex) {
vector<string> v;
vector<QV, trace_allocator<QV>> v0(vm);
auto citr = copyVisitor(std::back_inserter(v0));
at(stackIndex).copyInto(*this, citr);
for (QV x: v0) v.push_back(x.asString());
return v;
}

void QFiber::loadGlobal (const string& name) {
int symbol = vm.findGlobalSymbol(name, 0);
if (symbol<0) pushNull();
else push(vm.globalVariables.at(symbol));
}

void QFiber::storeGlobal (const string& name, bool isConst) {
vm.bindGlobal(name, top(), isConst);
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
for (auto bc = func.bytecode, end = func.bytecodeEnd; bc<end; ) {
uint8_t op = *bc++;
switch(op) {
case OP_LOAD_THIS_FIELD:
case OP_STORE_THIS_FIELD:
case OP_LOAD_FIELD:
case OP_STORE_FIELD:
*reinterpret_cast<uint_field_index_t*>(const_cast<char*>(bc)) += offset;
break;
}
bc += OPCODE_INFO[op].szArgs;
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
vm.imports[name] = at(-1);
}

void QFiber::import (const std::string& baseFile, const std::string& requestedFile) {
string finalFile = vm.pathResolver(baseFile, requestedFile);
if (vm.importHook(*this, finalFile, Swan::VM::ImportHookState::IMPORT_REQUEST, 0)) return;
auto it = vm.imports.find(finalFile);
if (it!=vm.imports.end()) push(it->second);
else {
vm.imports[finalFile] = true;
try {
if (!vm.importHook(*this, finalFile, Swan::VM::ImportHookState::BEFORE_IMPORT, 0)) {
int count = loadFile(finalFile);
vm.importHook(*this, finalFile, Swan::VM::ImportHookState::BEFORE_RUN, count);
while(--count>=0) {
const QClosure* closure = at(-1).isClosure()? at(-1).asObject<QClosure>() : nullptr;
const QFunction* func = closure? &closure->func : (at(-1).isNormalFunction()? at(-1).asObject<QFunction>() : nullptr);
string funcFile = func? func->file.str() : "";
call(0); 
if (funcFile.size()) vm.imports[funcFile] = top();
if (count>0) pop(); 
}}
vm.importHook(*this, finalFile, Swan::VM::ImportHookState::AFTER_RUN, 0);
vm.imports[finalFile] = top();
} catch (...) { 
vm.imports.erase(finalFile); 
throw; 
}
}}
