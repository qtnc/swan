#ifndef _____SWAN_STD_FUNCTION_HPP_____
#define _____SWAN_STD_FUNCTION_HPP_____
#include "Object.hpp"
#include "Value.hpp"

struct StdFunction: QObject {
typedef std::function<void(Swan::Fiber&)> Func;
Func func;
StdFunction (QVM& vm, const Func& func);
virtual ~StdFunction () = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
