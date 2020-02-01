#ifndef _____SWANLIB_HPP_____
#define _____SWANLIB_HPP_____
#include "../vm/VM.hpp"
#include "../vm/FiberVM.hpp"

static inline void doNothing (QFiber& f) { }

bool isName (uint32_t c);
bool isDigit (uint32_t c);

#endif
