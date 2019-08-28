#ifndef _____SWAN_FUNCTION_HPP_____
#define _____SWAN_FUNCTION_HPP_____
#include "Object.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include<string>
#include<vector>

struct Upvariable {
int slot;
bool upperUpvalue;
};

struct QFunction: QObject {
std::string bytecode, name, file;
std::vector<Upvariable, trace_allocator<Upvariable>> upvalues;
std::vector<QV, trace_allocator<QV>> constants;
uint_local_index_t nArgs;
bool vararg;

QFunction (QVM& vm);
virtual bool gcVisit () final override;
virtual ~QFunction () = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
