#ifndef _____SWAN_FUNCTION_HPP_____
#define _____SWAN_FUNCTION_HPP_____
#include "Object.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include<string>
#include<vector>

struct QFunction: QObject {
struct Upvalue {
int slot;
bool upperUpvalue;
};
std::string bytecode, name, file;
std::vector<Upvalue, trace_allocator<Upvalue>> upvalues;
std::vector<QV, trace_allocator<QV>> constants;
uint8_t nArgs;
bool vararg;

QFunction (QVM& vm);
virtual bool gcVisit () final override;
virtual ~QFunction () = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
