#include "Fiber.hpp"
#include "VM.hpp"
#include "OpCodeInfo.hpp"
#include "ExtraAlgorithms.hpp"
#include "../../include/cpprintf.hpp"
#include<boost/core/demangle.hpp>
using namespace std;

void pushSwanExceptionFromCppException (QFiber& f, const std::exception& e) {
string eName = "Error";
auto& tp = typeid(e);
boost::core::scoped_demangled_name exceptionType(tp.name());
if (tp==typeid(std::range_error) || tp==typeid(std::out_of_range) || tp==typeid(std::length_error) || tp==typeid(std::overflow_error) || tp==typeid(std::underflow_error)) eName = "RangeError";
else if (tp==typeid(std::domain_error) || tp==typeid(std::invalid_argument)) eName = "ValueError";
else if (tp==typeid(std::bad_cast) || tp==typeid(std::bad_function_call)) eName = "TypeError";
else if (tp==typeid(std::bad_function_call) || tp==typeid(call_error)) eName = "CallError";
f.loadGlobal(eName);
f.pushString(format("%s: %s", exceptionType.get(), e.what()));
f.call(1);
}

static void buildStackTraceLine (QCallFrame& frame, std::vector<Swan::StackTraceElement>& stackTrace) {
if (!frame.closure) return;
const QFunction& func = frame.closure->func;
int line = -1;
for (const char *bc = func.bytecode.data(), *end = func.bytecode.data()+func.bytecode.length(); bc<frame.bcp && bc<end; ) {
uint8_t op = *bc++;
if (op==OP_DEBUG_LINE) line = *reinterpret_cast<const int16_t*>(bc);
bc += OPCODE_INFO[op].nArgs;
}
stackTrace.push_back({ func.name.str(), func.file.str(), line });
}



static void throwRuntimeException (QFiber& f, int frameIdx, const std::exception& e) {
boost::core::scoped_demangled_name demangled(typeid(e).name());
string eType = demangled.get()? demangled.get() : "unknown_exception";
Swan::RuntimeException rt(eType, e.what(), 0);
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
