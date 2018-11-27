#include "QValue.hpp"
#include "QValueExt.hpp"
#include "../include/cpprintf.hpp"
#include<algorithm>
#include<boost/core/demangle.hpp>
using namespace std;

extern OpCodeInfo OPCODE_INFO[];

const uint8_t* printOpCode (const uint8_t* bc);

static void printStack (ostream& out, QFiber::Stack& stack, int base);

thread_local QFiber* QFiber::curFiber = nullptr;

const char* OPCODE_NAMES[] = {
#define OP(name, stackEffect, nArgs, argFormat) #name
#include "QOpCodes.hpp"
#undef OP
, nullptr
};

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

QS::Fiber* export QS::VM::getActiveFiber () {
return QFiber::curFiber;
}

QS::VM* export QS::VM::createVM  () {
return new QVM();
}

QFiber* QVM::createFiber () {
return new QFiber(*this);
}

QFiber::QFiber (QVM& vm0): QSequence(vm0.fiberClass), vm(vm0), state(FiberState::INITIAL)
, stack([this](const QV* _old, const QV* _new){ adjustUpvaluePointers(_old, _new); }, 8) 
{
stack.reserve(8);
callFrames.reserve(4);
}

void QVM::bindGlobal (const string& name, const QV& value) {
int symbol = findGlobalSymbol(name, true);
insert_n(globalVariables, 1+symbol-globalVariables.size(), QV());
globalVariables.at(symbol) = value;
}

QClass* QVM::createNewClass (const string& name, vector<QV>& parents, int nStaticFields, int nFields, bool foreign) {
string metaName = name + ("MetaClass");
QClass* parent = parents[0].asObject<QClass>();
QClass* meta = QClass::create(*this, classClass, classClass, metaName, 0, nStaticFields);
QClass* cls = foreign?
new QForeignClass(*this, meta, parent, name, nFields):
QClass::create(*this, meta, parent, name, nStaticFields, nFields+parent->nFields);
for (auto& p: parents) cls->mergeMixinMethods( p.asObject<QClass>() );
meta->bind("()", instantiate);
return cls;
}

static void buildStackTraceLine (QCallFrame& frame, std::vector<QS::StackTraceElement>& stackTrace) {
if (!frame.closure) return;
const QFunction& func = frame.closure->func;
int line = -1;
for (const char *bc = func.bytecode.data(), *end = func.bytecode.data()+func.bytecode.length(); bc<frame.bcp && bc<end; ) {
uint8_t op = *bc++;
if (op==OP_DEBUG_LINE) line = *reinterpret_cast<const int16_t*>(bc);
bc += OPCODE_INFO[op].nArgs;
}
stackTrace.push_back({ func.name, func.file, line });
}

template<class... A> void QFiber::runtimeError (const char* msg, const A&... args) {
throw std::runtime_error(format(msg, args...));
}

void QFiber::call (int nArgs) {
pushCppCallFrame();
callCallable(nArgs);
popCppCallFrame();
}

void QFiber::callSymbol (int symbol, int nArgs) {
QV receiver = *(stack.end() -nArgs);
QClass& cls = receiver.getClass(vm);
QV method = cls.findMethod(symbol);
bool re = callMethod(method, nArgs);
if (!re) runtimeError("%s has no method %s", cls.name, vm.methodSymbols[symbol]);
}

void QFiber::callSuperSymbol (int symbol, int nArgs) {
uint32_t newStackBase = stack.size() -nArgs;
QV receiver = stack.at(newStackBase);
QClass* cls = receiver.getClass(vm) .parent;
QV method = cls->findMethod(symbol);
bool re = callMethod(method, nArgs);
if (!re) {
runtimeError("%s has no method %s", cls->name, vm.methodSymbols[symbol]);
}}

bool QFiber::callMethod (QV& method, int nArgs) {
uint32_t newStackBase = stack.size() -nArgs;
QV receiver = stack.at(newStackBase);
if (method.isNativeFunction()) {
QNativeFunction func = method.asNativeFunction();
callFrames.push_back({nullptr, nullptr, newStackBase});
func(*this);
stack.resize(newStackBase+1);
callFrames.pop_back();
return true;
}
else if (method.isClosure()) {
QClosure& closure = *method.asObject<QClosure>();
callClosure(closure, nArgs);
return true;
}
else {
stack.resize(newStackBase);
push(QV());
return false;
}}

void QFiber::callCallable (int nArgs) {
uint32_t newStackBase = stack.size() -nArgs;
QV& method = stack.at(newStackBase -1);
const QClass& cls = method.getClass(vm);
if (method.isNativeFunction()) {
QNativeFunction func = method.asNativeFunction();
callFrames.push_back({nullptr, nullptr, newStackBase});
func(*this);
stack.at(newStackBase -1) = stack.at(newStackBase);
stack.resize(newStackBase);
callFrames.pop_back();
}
else if (method.isClosure()) {
QClosure* closure = method.asObject<QClosure>();
stack.erase(stack.begin() + newStackBase -1);
callClosure(*closure, nArgs);
}
else if (method.isFiber()) {
QFiber& f = *method.asObject<QFiber>();
callFiber(f, nArgs);
stack.erase(stack.begin() + newStackBase -1);
}
else if (method.isBoundFunction()) {
BoundFunction& bf = *method.asObject<BoundFunction>();
stack.insert(stack.begin() + newStackBase, bf.object);
method = bf.method;
callCallable(nArgs+1);
}
else if (method.isGenericSymbolFunction()) {
uint_method_symbol_t symbol = method.asInt<uint_method_symbol_t>();
stack.erase(stack.end() -nArgs -1);
callSymbol(symbol, nArgs);
}
else if (method.isNull()) {
stack.resize(newStackBase -1);
push(QV());
runtimeError("%s isn't callable", method.getClass(vm).name);
}
else {
int symbol = vm.findMethodSymbol(("()"));
QClass& cls = method.getClass(vm);
callSymbol(symbol, nArgs+1);
}}

void QFiber::adjustArguments (int& nArgs, int nClosureArgs, bool vararg) {
if (nArgs>=nClosureArgs) {
if (vararg) {
QTuple* tuple = QTuple::create(vm, nArgs+1-nClosureArgs, &stack.at(stack.size() +nClosureArgs -nArgs -1));
stack.erase(stack.end() +nClosureArgs -nArgs -1, stack.end());
push(tuple);
}
else if (nArgs>nClosureArgs) stack.erase(stack.end() +nClosureArgs -nArgs, stack.end());
nArgs = nClosureArgs;
}
else {
while (nArgs<nClosureArgs) {
push(QV());
nArgs++;
}
if (vararg) {
pop();
push(QTuple::create(vm, 0, nullptr));
}}
}

void QFiber::callClosure (QClosure& closure, int nArgs) {
adjustArguments(nArgs, closure.func.nArgs, closure.func.vararg);
uint32_t newStackBase = stack.size() -nArgs;
bool doRun = callFrames.back().isCppCallFrame();
callFrames.push_back({&closure, closure.func.bytecode.data(), newStackBase});
if (doRun) run();
}

void QFiber::callFiber (QFiber& f, int nArgs) {
switch(f.state){
case FiberState::INITIAL: {
QClosure& closure = *f.callFrames.back().closure;
f.stack.insert(f.stack.end(), stack.end() -nArgs, stack.end());
stack.erase(stack.end() -nArgs, stack.end());
f.adjustArguments(nArgs, closure.func.nArgs, closure.func.vararg);
curFiber = &f;
f.run();
curFiber = this;
}break;
case FiberState::YIELDED:
if (nArgs>=1) {
f.push(top());
stack.erase(stack.end() -nArgs, stack.end());
}
else f.push(QV());
curFiber = &f;
f.run();
curFiber = this;
break;
case FiberState::RUNNING:
case FiberState::FINISHED:
case FiberState::FAILED:
default:
runtimeError(("Couldn't call a running or finished fiber"));
push(QV());
return;
}
push(f.top());
f.pop();
}

Upvalue* QFiber::captureUpvalue (int slot) {
QV val = QV(QV_TAG_OPEN_UPVALUE | reinterpret_cast<uintptr_t>(&stack.at(callFrames.back().stackBase + slot)));
QFiber* _this=this;
auto it = find_if(openUpvalues.begin(), openUpvalues.end(), [&](auto x){ return x->fiber==_this && x->value.i==val.i; });
if (it!=openUpvalues.end()) return *it;
auto upvalue = new Upvalue(*this, slot);
openUpvalues.push_back(upvalue);
return upvalue;
}

void QFiber::closeUpvalues (int startSlot) {
QFiber* _this=this;
const void *ptrBeg = &stack[startSlot], *ptrEnd = &stack[stack.size() -1];
auto newEnd = remove_if(openUpvalues.begin(), openUpvalues.end(), [&](auto upvalue){
QV v0 = upvalue->value;
const void* ptr = v0.asPointer<char>();
if (upvalue->fiber==_this
&& upvalue->value.isOpenUpvalue() 
&& ptr >= ptrBeg
&& ptr < ptrEnd
) {
upvalue->close();
return true;
}
else return false;
});
openUpvalues.erase(newEnd, openUpvalues.end());
}

void QFiber::adjustUpvaluePointers (const QV* oldPtr, const QV* newPtr) {
if (!oldPtr) return;
for (auto& upvalue: openUpvalues) {
upvalue->value.i += (static_cast<int64_t>(reinterpret_cast<uintptr_t>(newPtr)) - static_cast<int64_t>(reinterpret_cast<uintptr_t>(oldPtr)));
}}

void QFiber::pushNewClass (int nParents, int nStaticFields, int nFields) {
string name = at(-nParents -1).asString();
vector<QV> parents(stack.end() -nParents, stack.end());
QClass* cls = vm.createNewClass(name, parents, nStaticFields, nFields, false);
stack.erase(stack.end() -nParents -1, stack.end());
push(cls);
}

void QFiber::pushNewForeignClass (const std::string& name, size_t id, int nUserBytes, int nParents) {
if (nParents<=0) {
push(vm.objectClass);
nParents = 1;
}
vector<QV> parents(stack.end() -nParents, stack.end());
QForeignClass* cls = static_cast<QForeignClass*>(vm.createNewClass(name, parents, 0, nUserBytes, true));
cls->id = id;
stack.erase(stack.end() -nParents, stack.end());
push(cls);
vm.foreignClassIds[id] = cls;
}

void QFiber::callMethod (const string& name, int nArgs) {
int symbol = vm.findMethodSymbol(name);
pushCppCallFrame();
callSymbol(symbol, nArgs);
popCppCallFrame();
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

static QV loadMethodSymbol (QVM& vm, QV& obj, int symbol) {
QClass& cls = obj.getClass(vm);
if (cls.isSubclassOf(vm.classClass)) {
QClass& c = *obj.asObject<QClass>();
if (symbol<c.methods.size() && !c.methods[symbol].isNull()) return c.methods[symbol];
else if (symbol<cls.methods.size() && !cls.methods[symbol].isNull()) return cls.methods[symbol];
}
else if (symbol<cls.methods.size() && !cls.methods[symbol].isNull()) {
return QV(new BoundFunction(vm, obj, cls.methods[symbol]), QV_TAG_BOUND_FUNCTION);
}
return QV();
}

static void adjustFieldOffset (QFunction& func, int offset) {
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

inline void QFiber::storeStaticMethod (int symbol) {
at(-2).asObject<QClass>() ->type->bind(symbol, top());
}

inline void QFiber::storeMethod (int symbol) {
at(-2).asObject<QClass>() ->bind(symbol, top());
if (top().isClosure()) adjustFieldOffset(top().asObject<QClosure>()->func, at(-2).asObject<QClass>()->parent->nFields);
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

int QVM::getOption (QVM::Option opt) {
switch(opt){
case Option::VAR_DECL_MODE: return varDeclMode;
default: return 0;
}}

void QVM::setOption (QVM::Option opt, int value) {
switch(opt){
case Option::VAR_DECL_MODE:
varDeclMode = value;
break;
}}

static inline int countArgsToMark (QFiber::Stack& stack) {
int count = 0;
for (int i=stack.size() -1; i>=0; i--) {
if (stack[i].i == QV_VARARG_MARK) break;
count++;
}
return count;
}

static void printStack (ostream& out, QFiber::Stack& stack, int base) {
print(out, "Stack base=%d, size=%d: [", base, stack.size());
for (int i=base, n=stack.size(); i<n; i++) {
if (i>0) print(out, ", ");
print(out, "%s", stack.at(i).print());
}
println(out, "");
}

static void throwRuntimeException (QFiber& f, int frameIdx, const std::exception& e) {
boost::core::scoped_demangled_name demangled(typeid(e).name());
string eType = demangled.get()? demangled.get() : "unknown_exception";
QS::RuntimeException rt(eType, e.what(), 0);
for (auto it=f.callFrames.begin() + frameIdx, end=f.callFrames.end(); it!=end; ++it) buildStackTraceLine(*it, rt.stackTrace);
f.stack.erase(f.stack.begin() + f.callFrames[frameIdx].stackBase, f.stack.end());
f.callFrames.erase(f.callFrames.begin() + frameIdx, f.callFrames.end());
throw rt;
}

void QFiber::handleException (const std::exception& e) {
if (catchPoints.empty()) {
auto lastCppFrame = find_last_if(callFrames.begin(), callFrames.end() -1, [](auto& c){ return c.isCppCallFrame(); });
if (lastCppFrame==callFrames.end()) lastCppFrame=callFrames.begin();
throwRuntimeException(*this, lastCppFrame - callFrames.begin(), e);
}
else {
auto& catchPoint = catchPoints.back();
if (catchPoint.callFrame <= callFrames.size() -1) {
auto lastCppFrame = find_last_if(callFrames.begin() + catchPoint.callFrame, callFrames.end() -1, [](auto& c){ return c.isCppCallFrame(); });
if (lastCppFrame!=callFrames.end() -1) throwRuntimeException(*this, lastCppFrame - callFrames.begin(), e);
}
stack.erase(stack.begin() + catchPoint.stackSize, stack.end() -1);
callFrames.erase(callFrames.begin() + catchPoint.callFrame, callFrames.end());
auto& frame = callFrames.back();
if (catchPoint.catchBlock==0xFFFFFFFF) {
state = FiberState::FAILED;
frame.bcp = frame.closure->func.bytecode.data() + catchPoint.finallyBlock;
}
else {
state = FiberState::RUNNING;
frame.bcp = frame.closure->func.bytecode.data() + catchPoint.catchBlock;
}
}}

FiberState QFiber::run ()  {
if (!curFiber) {
LOCK_SCOPE(vm.globalMutex)
vm.fiberThreads.push_back(&curFiber);
}
LOCK_SCOPE(mutex)
curFiber = this;
state = FiberState::RUNNING;
auto frame = callFrames.back();
uint8_t op;
int arg1, arg2;
//println("Running closure at %#P, stackBase=%d, closure.func.nArgs=%d, vararg=%d", frame.closure, frame.stackBase, frame.closure->func.nArgs, frame.closure->func.vararg);
begin: try {
//printStack(std::cout, stack, frame.stackBase);
//printOpCode(reinterpret_cast<const uint8_t*>(frame.bcp));
#ifdef USE_COMPUTED_GOTO
static const void* JUMP_TABLE[] = {
#define OP(NAME, UNUSED1, UNUSED2, UNUSED3) &&LABEL_OP_##NAME
#include "QOpCodes.hpp"
#undef OP
};
#define CASE(NAME) LABEL_##NAME:
#define BREAK op = frame.read<uint8_t>(); goto *JUMP_TABLE[op];
#define DEFAULT
#define SWITCH BREAK
#define END_SWITCH
#else
#define CASE(NAME) case NAME:
#define BREAK break;
#define DEFAULT default:
#define END_SWITCH }}
#define SWITCH while(true){\
op = frame.read<uint8_t>();\
switch(op){
#endif

SWITCH 
CASE(OP_LOAD_NULL)
push(QV());
BREAK

CASE(OP_LOAD_TRUE)
push(true);
BREAK

CASE(OP_LOAD_FALSE)
push(false);
BREAK

CASE(OP_LOAD_INT8)
push(static_cast<double>(frame.read<int8_t>()));
BREAK

CASE(OP_LOAD_CONSTANT)
push(frame.closure->func.constants[frame.read<uint_constant_index_t>()]);
BREAK

CASE(OP_LOAD_THIS)
push(stack.at(frame.stackBase));
BREAK

CASE(OP_LOAD_LOCAL)
push(stack.at(frame.stackBase + frame.read<uint_local_index_t>()));
BREAK

CASE(OP_LOAD_LOCAL_0)
CASE(OP_LOAD_LOCAL_1)
CASE(OP_LOAD_LOCAL_2)
CASE(OP_LOAD_LOCAL_3)
CASE(OP_LOAD_LOCAL_4)
CASE(OP_LOAD_LOCAL_5)
CASE(OP_LOAD_LOCAL_6)
CASE(OP_LOAD_LOCAL_7)
push(stack.at(frame.stackBase + op -OP_LOAD_LOCAL_0));
BREAK

CASE(OP_LOAD_GLOBAL)
push(vm.globalVariables.at(frame.read<uint_global_symbol_t>()));
BREAK

CASE(OP_LOAD_UPVALUE)
push(frame.closure->upvalues[frame.read<uint_upvalue_index_t>()]->get());
BREAK

CASE(OP_LOAD_FIELD)
push(at(0).asObject<QInstance>() ->fields[frame.read<uint_field_index_t>()]);
BREAK

CASE(OP_LOAD_STATIC_FIELD)
push( at(0).getClass(vm) .staticFields[frame.read<uint_field_index_t>()] );
BREAK

CASE(OP_LOAD_METHOD)
top() = loadMethodSymbol(vm, top(), frame.read<uint_method_symbol_t>());
BREAK

CASE(OP_STORE_LOCAL)
stack.at(frame.stackBase + frame.read<uint_local_index_t>()) = top();
BREAK

CASE(OP_STORE_LOCAL_0)
CASE(OP_STORE_LOCAL_1)
CASE(OP_STORE_LOCAL_2)
CASE(OP_STORE_LOCAL_3)
CASE(OP_STORE_LOCAL_4)
CASE(OP_STORE_LOCAL_5)
CASE(OP_STORE_LOCAL_6)
CASE(OP_STORE_LOCAL_7)
stack.at(frame.stackBase + op -OP_STORE_LOCAL_0) = top();
BREAK

CASE(OP_STORE_GLOBAL)
vm.globalVariables.at(frame.read<uint_global_symbol_t>()) = top();
BREAK

CASE(OP_STORE_UPVALUE)
frame.closure->upvalues[frame.read<uint_upvalue_index_t>()]->get() = top();
BREAK

CASE(OP_STORE_FIELD)
at(0).asObject<QInstance>() ->fields[frame.read<uint_field_index_t>()] = top();
BREAK

CASE(OP_STORE_STATIC_FIELD)
at(0).getClass(vm) .staticFields[frame.read<uint_field_index_t>()] = top();
BREAK

CASE(OP_STORE_METHOD)
storeMethod(frame.read<uint_method_symbol_t>());
BREAK

CASE(OP_STORE_STATIC_METHOD)
storeStaticMethod(frame.read<uint_method_symbol_t>());
BREAK

CASE(OP_POP)
pop();
BREAK

CASE(OP_DUP)
push(top());
BREAK

CASE(OP_LOAD_CLOSURE) {
QV& val = frame.closure->func.constants[frame.read<uint_constant_index_t>()];
QFunction& func = *val.asObject<QFunction>();
QClosure* closure = newVLS<QClosure, Upvalue*>(func.upvalues.size(), vm, func);
for (int i=0, n=func.upvalues.size(); i<n; i++) {
auto& upvalue = func.upvalues[i];
closure->upvalues[i] = upvalue.upperUpvalue? closure->upvalues[upvalue.slot] : captureUpvalue(upvalue.slot);
}
push(QV(closure, QV_TAG_CLOSURE));
}
BREAK

CASE(OP_AND)
arg1 = frame.read<uint_jump_offset_t>();
if (top().isFalsy()) frame.bcp += arg1;
else pop();
BREAK

CASE(OP_OR)
arg1 = frame.read<uint_jump_offset_t>();
if (!top().isFalsy()) frame.bcp += arg1;
else pop();
BREAK

CASE(OP_NULL_COALESCING)
arg1 = frame.read<uint_jump_offset_t>();
if (!top().isNull()) frame.bcp += arg1;
else pop();
BREAK

CASE(OP_JUMP)
frame.bcp += frame.read<uint_jump_offset_t>();
BREAK

CASE(OP_JUMP_BACK)
frame.bcp -= frame.read<uint_jump_offset_t>();
BREAK

CASE(OP_JUMP_IF_FALSY)
arg1 = frame.read<uint_jump_offset_t>();
if (top().isFalsy()) frame.bcp += arg1;
pop();
BREAK

CASE(OP_NEW_CLASS) {
int nParents = frame.read<uint_field_index_t>();
int nStaticFields = frame.read<uint_field_index_t>();
int nFields = frame.read<uint_field_index_t>();
pushNewClass(nParents, nStaticFields, nFields);
}
BREAK

#define C(N) CASE(OP_CALL_METHOD_##N)
C(0) C(1) C(2) C(3) C(4) C(5) C(6) C(7) C(8)
C(9) C(10) C(11) C(12) C(13) C(14) C(15)
#undef C
arg1 = frame.read<uint_method_symbol_t>();
callFrames.back() = frame;
callSymbol(arg1, op - OP_CALL_METHOD_0);
frame = callFrames.back();
BREAK

#define C(N) CASE(OP_CALL_SUPER_##N)
C(0) C(1) C(2) C(3) C(4) C(5) C(6) C(7) C(8)
C(9) C(10) C(11) C(12) C(13) C(14) C(15)
#undef C
arg1 = frame.read<uint_method_symbol_t>();
callFrames.back() = frame;
callSuperSymbol(arg1, op - OP_CALL_SUPER_0);
frame = callFrames.back();
BREAK

#define C(N) CASE(OP_CALL_FUNCTION_##N)
C(0) C(1) C(2) C(3) C(4) C(5) C(6) C(7) C(8)
C(9) C(10) C(11) C(12) C(13) C(14) C(15)
#undef C
callFrames.back() = frame;
callCallable(op - OP_CALL_FUNCTION_0);
frame = callFrames.back();
BREAK

CASE(OP_CALL_METHOD)
arg1 = frame.read<uint_method_symbol_t>();
arg2 = frame.read<uint8_t>();
callFrames.back() = frame;
callSymbol(arg1, arg2);
frame = callFrames.back();
BREAK

CASE(OP_CALL_SUPER)
arg1 = frame.read<uint_method_symbol_t>();
arg2 = frame.read<uint8_t>();
callFrames.back() = frame;
callSuperSymbol(arg1, arg2);
frame = callFrames.back();
BREAK

CASE(OP_CALL_FUNCTION)
arg2 = frame.read<uint8_t>();
callFrames.back() = frame;
callCallable(arg2);
frame = callFrames.back();
BREAK

CASE(OP_CALL_METHOD_VARARG)
arg2 = countArgsToMark(stack);
arg1 = frame.read<uint_method_symbol_t>();
callFrames.back() = frame;
callSymbol(arg1, arg2);
stack.erase(stack.end() -2);
frame = callFrames.back();
BREAK

CASE(OP_CALL_SUPER_VARARG)
arg2 = countArgsToMark(stack);
arg1 = frame.read<uint_method_symbol_t>();
callFrames.back() = frame;
callSuperSymbol(arg1, arg2);
stack.erase(stack.end() -2);
frame = callFrames.back();
BREAK

CASE(OP_CALL_FUNCTION_VARARG)
arg2 = countArgsToMark(stack) -1;
callFrames.back() = frame;
callCallable(arg2);
stack.erase(stack.end() -2);
frame = callFrames.back();
BREAK

CASE(OP_PUSH_VARARG_MARK)
push(QV(QV_VARARG_MARK));
BREAK

CASE(OP_UNPACK_SEQUENCE) {
QV val = top();
pop();
vector<QV> buffer;
val.asObject<QSequence>()->insertIntoVector(*this, buffer, 0);
stack.insert(stack.end(), buffer.begin(), buffer.end());
}
BREAK

CASE(OP_POP_SCOPE) {
int newSize = stack.size() - frame.read<uint_local_index_t>();
closeUpvalues(newSize);
stack.resize(newSize);
}
BREAK

CASE(OP_THROW)
handleException(std::runtime_error(ensureString(-1)->asString()));
frame = callFrames.back();
BREAK

CASE(OP_TRY)
arg1 = frame.read<uint32_t>();
arg2 = frame.read<uint32_t>();
catchPoints.push_back({ stack.size(), callFrames.size(), static_cast<size_t>(arg1), static_cast<size_t>(arg2) });
BREAK

CASE(OP_END_FINALLY)
catchPoints.pop_back();
if (state==FiberState::FAILED) {
handleException(std::runtime_error(ensureString(-1)->asString()));
frame = callFrames.back();
}
BREAK

CASE(OP_DEBUG_LINE)
frame.read<uint16_t>();
BREAK

CASE(OP_RETURN)
closeUpvalues(frame.stackBase);
stack.at(frame.stackBase) = top();
stack.resize(frame.stackBase +1);
callFrames.pop_back();
//print("Returning from closure, ");
//printStack(std::cout, stack, 0);
if (callFrames.empty() || callFrames.back().isCppCallFrame()) {
//println("Fiber finished executing");
state = FiberState::FINISHED;
return state;
}
else frame = callFrames.back();
//printStack(std::cout, stack, 0);
//println("frame .closure=%p, .bcp=%p, .base=%d", frame.closure, reinterpret_cast<uintptr_t>(frame.bcp), frame.stackBase);
BREAK

CASE(OP_YIELD)
//print("Yielding from closure, ");
//printStack(std::cout, stack, 0);
callFrames.back() = frame;
return state = FiberState::YIELDED;

CASE(OP_END)
DEFAULT
state = FiberState::FAILED;
throw std::runtime_error(format("Invalid opcode: %#0$2X", static_cast<int>(op)));
END_SWITCH
} //end try
catch (QS::RuntimeException& e) {
state = FiberState::FAILED;
throw;
}
catch (std::exception& e) {
boost::core::scoped_demangled_name exceptionType(typeid(e).name());
string s = format("%s: %s", exceptionType.get(), e.what());
pushString(s);
handleException(e);
frame = callFrames.back();
goto begin;
}
catch(...) {
pushString("Unknown C++ exception");
handleException(std::runtime_error("Unknown C++ exception"));
frame = callFrames.back();
goto begin;
}
#undef SWITCH
#undef CASE
#undef BREAK
#undef DEFAULT
#undef END_SWITCH
}
