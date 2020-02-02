#ifndef _____SWAN_STD_FUNCTION_HPP_____
#define _____SWAN_STD_FUNCTION_HPP_____
#include "Object.hpp"
#include "Value.hpp"

struct StdFunction: QObject {
typedef std::function<void(Swan::Fiber&)> Func;
Func func;
StdFunction (QVM& vm, const Func& func);
~StdFunction () = default;
inline size_t getMemSize () { return sizeof(*this); }
};

#endif
