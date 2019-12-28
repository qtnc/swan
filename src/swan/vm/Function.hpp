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
QV* constants;
union { Upvariable* upvalues; QV* constantsEnd; };
union { char *bytecode; Upvariable* upvaluesEnd; };
char* bytecodeEnd;
c_string name, file;
struct QClass* returnType;
uint_local_index_t nArgs;
bool vararg;

static QFunction* create (QVM& vm, int nConstants, int nUpvalues, int bcSize);
QFunction (QVM& vm);
virtual bool gcVisit () final override;
virtual ~QFunction () = default;
virtual size_t getMemSize () override { return  bytecodeEnd - reinterpret_cast<char*>(this); }
};

#endif
