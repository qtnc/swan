#ifndef _____FIBER_INLINES_____
#define _____FIBER_INLINES_____
#include "Fiber.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"

void adjustFieldOffset (QFunction& func, int offset);

inline void QFiber::storeMethod (int symbol) {
at(-2).asObject<QClass>() ->bind(symbol, top());
if (top().isClosure()) adjustFieldOffset(top().asObject<QClosure>()->func, at(-2).asObject<QClass>()->parent->nFields);
}

inline void QFiber::storeStaticMethod (int symbol) {
at(-2).asObject<QClass>() ->type->bind(symbol, top());
}

#endif
