#include "Fiber.hpp"
#include "VM.hpp"
#include "Upvalue.hpp"
#include<string>
using namespace std;

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


