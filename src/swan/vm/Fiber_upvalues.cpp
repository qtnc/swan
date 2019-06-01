#include "Fiber.hpp"
#include "VM.hpp"
#include "Upvalue.hpp"
#include<string>
#include "../../include/cpprintf.hpp"
using namespace std;

void QFiber::closeUpvalues (int startSlot) {
const QV *ptrBeg = &stack[startSlot], *ptrEnd = &stack[stack.size()];
auto newEnd = remove_if(openUpvalues.begin(), openUpvalues.end(), [&](auto upvalue){
QV v0 = upvalue->value;
const QV *ptr = v0.asPointer<QV>();
if (upvalue->fiber==this
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
//print(std::cout, "Move upvalue %s: %#0$16llX ", upvalue->get().print(), upvalue->value.i);
upvalue->value.i = (upvalue->value.i - reinterpret_cast<uintptr_t>(oldPtr)) + reinterpret_cast<uintptr_t>(newPtr);
//println(" => %#0$16llX", upvalue->value.i);
}}


