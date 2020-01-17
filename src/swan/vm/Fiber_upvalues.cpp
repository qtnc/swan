#include "Fiber.hpp"
#include "VM.hpp"
#include "Upvalue.hpp"
#include<string>
#include "../../include/cpprintf.hpp"
using namespace std;

void QFiber::closeUpvalues (int startSlot) {
const QV *ptrBeg = &stack[startSlot], *ptrEnd = &stack[stack.size()];
auto newEnd = remove_if(openUpvalues.begin(), openUpvalues.end(), [&](auto upvalue){
const QV *ptr = upvalue->value;
if (upvalue->fiber==this
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
upvalue->value = reinterpret_cast<QV*>((reinterpret_cast<uintptr_t>(upvalue->value) - reinterpret_cast<uintptr_t>(oldPtr)) + reinterpret_cast<uintptr_t>(newPtr));
}}


