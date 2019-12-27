#ifndef _____SWAN_FUNCTION_HPP_____
#define _____SWAN_FUNCTION_HPP_____
#include "Object.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include "Array.hpp"
#include<string>
#include<vector>

struct Upvariable {
int slot;
bool upperUpvalue;
};

struct QFunction: QObject {
simple_string bytecode, name, file;
simple_array<Upvariable, trace_allocator<char>> upvalues;
simple_array<QV, trace_allocator<char>> constants;
uint_local_index_t nArgs;
bool vararg;

QFunction (QVM& vm);
virtual bool gcVisit () final override;
virtual ~QFunction () = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
