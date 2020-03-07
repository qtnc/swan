#include "Fiber.hpp"
#include "VM.hpp"
#include "Fiber_inlines.hpp"
#include "OpCodeInfo.hpp"
#include "Upvalue.hpp"
#include "BoundFunction.hpp"
#include "../../include/cpprintf.hpp"
#include<cmath>
using namespace std;

double dlshift (double, double);
double drshift (double, double);
double dintdiv (double, double);

void pushSwanExceptionFromCppException (QFiber& f, const std::exception& e);

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

static void printStack (QFiber::Stack& stack, int base) {
printStack(std::cerr, stack, base);
}

QV QFiber::loadMethod (QV& obj, int symbol) {
QClass& cls = obj.getClass(vm);
if (cls.isSubclassOf(vm.classClass)) {
QClass& c = *obj.asObject<QClass>();
if (symbol<c.methods.size() && !c.methods[symbol].isNull()) return c.methods[symbol];
else if (symbol<cls.methods.size() && !cls.methods[symbol].isNull()) return cls.methods[symbol];
}
else if (symbol<cls.methods.size() && !cls.methods[symbol].isNullOrUndefined()) {
return QV(BoundFunction::create(vm, cls.methods[symbol], 1, &obj), QV_TAG_BOUND_FUNCTION);
}
return QV::UNDEFINED;
}


FiberState QFiber::run ()  {
//println(std::cerr, "Running fiber: parent=%s, cur=%s, this=%s", (parentFiber? QV(parentFiber, QV_TAG_FIBER) : QV()).print(), (curFiber? QV(curFiber, QV_TAG_FIBER) : QV()).print(), QV(this, QV_TAG_FIBER).print() );
vm.activeFiber = this;
state = FiberState::RUNNING;
auto  frame = callFrames.back();
auto& stack = this->stack;
uint8_t op;
int arg1, arg2;
//println("Running closure at %#P, stackBase=%d, closure.func.nArgs=%d, vararg=%d", frame.closure, frame.stackBase, frame.closure->func.nArgs, frame.closure->func.vararg);
begin: try {
//printStack(std::cout, stack, frame.stackBase);
//printOpCode(reinterpret_cast<const uint8_t*>(frame.bcp));
#define USE_COMPUTED_GOTO
#ifdef USE_COMPUTED_GOTO
static const void* JUMP_TABLE[] = {
#define OP(NAME, UNUSED1, UNUSED2, UNUSED3) &&LABEL_OP_##NAME
#include "OpCodes.hpp"
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

CASE(OP_LOAD_LOCAL_0)
CASE(OP_LOAD_LOCAL_1)
CASE(OP_LOAD_LOCAL_2)
CASE(OP_LOAD_LOCAL_3)
CASE(OP_LOAD_LOCAL_4)
CASE(OP_LOAD_LOCAL_5)
CASE(OP_LOAD_LOCAL_6)
CASE(OP_LOAD_LOCAL_7)
stack.push(stack.at(frame.stackBase + op -OP_LOAD_LOCAL_0));
BREAK

CASE(OP_LOAD_LOCAL)
stack.push(stack.at(frame.stackBase + frame.read<uint_local_index_t>()));
BREAK

CASE(OP_LOAD_THIS)
stack.push(stack.at(frame.stackBase));
BREAK

CASE(OP_LOAD_THIS_FIELD)
stack.push(stack.at(frame.stackBase).asObject<QInstance>() ->fields[frame.read<uint_field_index_t>()]);
BREAK

CASE(OP_POP)
stack.pop();
BREAK

CASE(OP_DUP)
stack.push(stack.back());
BREAK

CASE(OP_LOAD_UNDEFINED)
stack.push(QV::UNDEFINED);
BREAK

CASE(OP_LOAD_NULL)
stack.push(QV::Null);
BREAK

CASE(OP_LOAD_TRUE)
stack.push(true);
BREAK

CASE(OP_LOAD_FALSE)
stack.push(false);
BREAK

CASE(OP_LOAD_INT8)
stack.push(static_cast<double>(frame.read<int8_t>()));
BREAK

CASE(OP_LOAD_CONSTANT)
stack.push(frame.closure->func.constants[frame.read<uint_constant_index_t>()]);
BREAK

CASE(OP_STORE_LOCAL_0)
CASE(OP_STORE_LOCAL_1)
CASE(OP_STORE_LOCAL_2)
CASE(OP_STORE_LOCAL_3)
CASE(OP_STORE_LOCAL_4)
CASE(OP_STORE_LOCAL_5)
CASE(OP_STORE_LOCAL_6)
CASE(OP_STORE_LOCAL_7)
stack.at(frame.stackBase + op -OP_STORE_LOCAL_0) = stack.back();
BREAK

CASE(OP_STORE_LOCAL)
stack.at(frame.stackBase + frame.read<uint_local_index_t>()) = stack.back();
BREAK

#define C(N) CASE(OP_CALL_METHOD_##N)
C(0) C(1) C(2) C(3) C(4) C(5) C(6) C(7)
#undef C
arg1 = frame.read<uint_method_symbol_t>();
this->callFrames.back() = frame;
callSymbol(arg1, op - OP_CALL_METHOD_0);
frame = this->callFrames.back();
BREAK

#define C(N) CASE(OP_CALL_SUPER_##N)
C(0) C(1) C(2) C(3) C(4) C(5) C(6) C(7)
#undef C
arg1 = frame.read<uint_method_symbol_t>();
this->callFrames.back() = frame;
callSuperSymbol(arg1, op - OP_CALL_SUPER_0);
frame = this->callFrames.back();
BREAK

#define C(N) CASE(OP_CALL_FUNCTION_##N)
C(0) C(1) C(2) C(3) C(4) C(5) C(6) C(7)
#undef C
this->callFrames.back() = frame;
callCallable(op - OP_CALL_FUNCTION_0);
frame = this->callFrames.back();
BREAK

CASE(OP_LOAD_UPVALUE)
stack.push(frame.closure->upvalues[frame.read<uint_upvalue_index_t>()]->get());
BREAK

CASE(OP_STORE_UPVALUE)
frame.closure->upvalues[frame.read<uint_upvalue_index_t>()]->get() = stack.back();
BREAK

CASE(OP_LOAD_GLOBAL)
stack.push(vm.globalVariables.at(frame.read<uint_global_symbol_t>()));
BREAK

CASE(OP_STORE_GLOBAL)
vm.globalVariables.at(frame.read<uint_global_symbol_t>()) = stack.back();
BREAK

CASE(OP_STORE_THIS_FIELD)
stack.at(frame.stackBase).asObject<QInstance>() ->fields[frame.read<uint_field_index_t>()] = stack.back();
BREAK

CASE(OP_LOAD_THIS_STATIC_FIELD)
stack.push( stack.at(frame.stackBase).getClass(vm) .staticFields[frame.read<uint_field_index_t>()] );
BREAK

CASE(OP_STORE_THIS_STATIC_FIELD)
stack.at(frame.stackBase).getClass(vm) .staticFields[frame.read<uint_field_index_t>()] = stack.back();
BREAK

CASE(OP_LOAD_FIELD)
stack.back() = stack.back().asObject<QInstance>() ->fields[frame.read<uint_field_index_t>()];
BREAK

CASE(OP_STORE_FIELD)
stack.back().asObject<QInstance>() ->fields[frame.read<uint_field_index_t>()] = stack.back(-2);
stack.pop();
BREAK

CASE(OP_LOAD_STATIC_FIELD)
stack.back() = stack.back().asObject<QClass>() ->staticFields[frame.read<uint_field_index_t>()] ;
BREAK

CASE(OP_STORE_STATIC_FIELD)
stack.back().asObject<QClass>() ->staticFields[frame.read<uint_field_index_t>()] = stack.back(-2);
stack.pop();
BREAK

#define G(N,Q) \
CASE(OP_##N) \
stack.pop(); Q; BREAK
#define OPA(N,O) G(N, stack.back().d O  stack.end()->d ) 
#define OPC(N,O) G(N, stack.back() = top().d O  stack.end()->d ) 
#define OPB(N,O) G(N, stack.back().d = static_cast<double>(static_cast<int64_t>(stack.back().d) O static_cast<int64_t>(stack.end()->d)) ) 
#define OPF(N,F) G(N, stack.back().d = F(stack.back().d, stack.end()->d) ) 
OPA(ADD, +=) OPA(SUB, -=)
OPA(MUL, *=) OPA(DIV, /=)
OPB(BINAND, &) OPB(BINOR, |) OPB(BINXOR, ^)
OPF(LSH, dlshift) OPF(RSH, drshift)
OPF(INTDIV, dintdiv) OPF(MOD, fmod) OPF(POW, pow)
OPC(EQ, ==) OPC(NEQ, !=)
OPC(LT, <) OPC(GT, >) OPC(LTE, <=) OPC(GTE, >=)
#undef OPA
#undef OPB
#undef OPC
#undef OPF
#undef G

CASE(OP_NEG)
stack.back().d = -stack.back().d;
BREAK

CASE(OP_BINNOT)
stack.back().d = static_cast<double>(~static_cast<int64_t>(stack.back().d));
BREAK

CASE(OP_NOT)
stack.back() = stack.back().isFalsy();
BREAK

CASE(OP_JUMP)
frame.bcp += frame.read<uint_jump_offset_t>();
BREAK

CASE(OP_JUMP_BACK)
frame.bcp -= frame.read<uint_jump_offset_t>();
BREAK

CASE(OP_JUMP_IF_FALSY)
arg1 = frame.read<uint_jump_offset_t>();
if (stack.back().isFalsy()) frame.bcp += arg1;
stack.pop();
BREAK

CASE(OP_JUMP_IF_TRUTY)
arg1 = frame.read<uint_jump_offset_t>();
if (!stack.back().isFalsy()) frame.bcp += arg1;
stack.pop();
BREAK

CASE(OP_JUMP_IF_UNDEFINED)
arg1 = frame.read<uint_jump_offset_t>();
if (stack.back().isUndefined()) {
frame.bcp += arg1;
stack.pop();
}
BREAK

CASE(OP_AND)
arg1 = frame.read<uint_jump_offset_t>();
if (stack.back().isFalsy()) frame.bcp += arg1;
else stack.pop();
BREAK

CASE(OP_OR)
arg1 = frame.read<uint_jump_offset_t>();
if (!stack.back().isFalsy()) frame.bcp += arg1;
else stack.pop();
BREAK

CASE(OP_NULL_COALESCING)
arg1 = frame.read<uint_jump_offset_t>();
if (!stack.back().isNullOrUndefined()) frame.bcp += arg1;
else stack.pop();
BREAK

CASE(OP_POP_SCOPE) 
arg1 = stack.size() - frame.read<uint_local_index_t>();
closeUpvalues(arg1);
stack.resize(arg1);
BREAK

CASE(OP_RETURN)
closeUpvalues(frame.stackBase);
stack.at(frame.stackBase) = stack.back();
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

CASE(OP_LOAD_METHOD)
stack.back() = loadMethod(stack.back(), frame.read<uint_method_symbol_t>());
BREAK

CASE(OP_STORE_METHOD)
storeMethod(frame.read<uint_method_symbol_t>());
BREAK

CASE(OP_STORE_STATIC_METHOD)
storeStaticMethod(frame.read<uint_method_symbol_t>());
BREAK

// CASE(OP_POP_M2)
//stack.erase(stack.end() -2);
//BREAK

//CASE(OP_DUP_M2)
//stack.push(stack.back(-2));
//BREAK

CASE(OP_SWAP) 
arg1 = frame.read<uint8_t>();
std::swap( stack.back(reinterpret_cast<int2x4_t*>(&arg1)->first), stack.back(reinterpret_cast<int2x4_t*>(&arg1)->second) );
BREAK

CASE(OP_CALL_METHOD)
arg1 = frame.read<uint_method_symbol_t>();
arg2 = frame.read<uint_local_index_t>();
this->callFrames.back() = frame;
callSymbol(arg1, arg2);
frame = this->callFrames.back();
BREAK

CASE(OP_CALL_SUPER)
arg1 = frame.read<uint_method_symbol_t>();
arg2 = frame.read<uint_local_index_t>();
this->callFrames.back() = frame;
callSuperSymbol(arg1, arg2);
frame = this->callFrames.back();
BREAK

CASE(OP_CALL_FUNCTION)
arg2 = frame.read<uint_local_index_t>();
this->callFrames.back() = frame;
callCallable(arg2);
frame = this->callFrames.back();
BREAK

CASE(OP_CALL_METHOD_VARARG)
arg2 = countArgsToMark(stack);
arg1 = frame.read<uint_method_symbol_t>();
this->callFrames.back() = frame;
callSymbol(arg1, arg2);
stack.erase(stack.end() -2);
frame = this->callFrames.back();
BREAK

CASE(OP_CALL_SUPER_VARARG)
arg2 = countArgsToMark(stack);
arg1 = frame.read<uint_method_symbol_t>();
this->callFrames.back() = frame;
stack.erase(stack.end() -arg2 -1);
callSuperSymbol(arg1, arg2 -1);
frame = this->callFrames.back();
BREAK

CASE(OP_CALL_FUNCTION_VARARG)
arg2 = countArgsToMark(stack) -1;
this->callFrames.back() = frame;
stack.erase(stack.end() -arg2 -2);
callCallable(arg2);
frame = this->callFrames.back();
BREAK

CASE(OP_PUSH_VARARG_MARK)
stack.push(QV(QV_VARARG_MARK));
BREAK

CASE(OP_UNPACK_SEQUENCE) 
unpackSequence();
BREAK

CASE(OP_LOAD_CLOSURE) 
loadPushClosure(frame.closure, frame.read<uint_constant_index_t>());
BREAK

CASE(OP_NEW_CLASS) {
int nParents = frame.read<uint_field_index_t>();
int nStaticFields = frame.read<uint_field_index_t>();
int nFields = frame.read<uint_field_index_t>();
pushNewClass(nParents, nStaticFields, nFields);
}
BREAK

CASE(OP_THROW) {
stack.push(stack.back());
std::runtime_error e(ensureString(-1)->asString());
stack.pop();
handleException(e);
frame = this->callFrames.back();
}BREAK

CASE(OP_TRY)
arg1 = frame.read<uint32_t>();
arg2 = frame.read<uint32_t>();
this->catchPoints.push_back({ stack.size(), callFrames.size(), static_cast<size_t>(arg1), static_cast<size_t>(arg2) });
BREAK

CASE(OP_END_FINALLY)
this->catchPoints.pop_back();
if (state==FiberState::FAILED) {
handleException(std::runtime_error(ensureString(-1)->asString()));
frame = this->callFrames.back();
}
BREAK

CASE(OP_DEBUG_LINE)
frame.read<uint16_t>();
BREAK

CASE(OP_DEBUG)
//printStack(cout, stack, frame.stackBase);
/*println("%s", stack.back().print() );
if (stack.back().isObject()) {
if (stack.back().isClosure()) {
auto f = stack.back().asObject<QClosure>();
f->func.printInstructions();
}}*/
BREAK

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
pushSwanExceptionFromCppException(*this, e);
handleException(e);
frame = this->callFrames.back();
goto begin;
}
catch(...) {
pushString("Unknown C++ exception");
handleException(std::runtime_error("Unknown C++ exception"));
frame = this->callFrames.back();
goto begin;
}
#undef SWITCH
#undef CASE
#undef BREAK
#undef DEFAULT
#undef END_SWITCH
}
