#include "Fiber.hpp"
#include "VM.hpp"
#include "Fiber_inlines.hpp"
#include "OpCodeInfo.hpp"
#include "Upvalue.hpp"
#include "BoundFunction.hpp"
#include "../../include/cpprintf.hpp"
using namespace std;

const char* OPCODE_NAMES[] = {
#define OP(name, stackEffect, nArgs, argFormat) #name
#include "OpCodes.hpp"
#undef OP
, nullptr
};

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
stack.erase(stack.end() -arg2 -2);
callCallable(arg2);
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
catch (Swan::RuntimeException& e) {
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
