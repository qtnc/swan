#ifndef _____SWANLIB_HPP_____
#define _____SWANLIB_HPP_____
#include "../vm/VM.hpp"

#define FUNC(BODY) [](QFiber& f){ BODY }
#define BIND_L(NAME, BODY) ->bind(#NAME, FUNC(BODY))
#define BIND_GL(NAME, BODY) bindGlobal(#NAME, QV(FUNC(BODY)));
#define BIND_F(NAME, F) ->bind(#NAME, F)
#define BIND_N(NAME) BIND_F(NAME, doNothing)

static inline void doNothing (QFiber& f) { }
bool isName (uint32_t c);
bool isDigit (uint32_t c);


#endif
