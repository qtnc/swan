#ifndef _____SWAN_FUNCTION_HPP_____
#define _____SWAN_FUNCTION_HPP_____
#include "Object.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include "Array.hpp"
#include<string>
#include<vector>

struct Upvariable {
uint_local_index_t slot;
bool upperUpvalue :1;
};

struct QFunction: QObject {
union { Upvariable* upvalues; QV* constantsEnd; };
union { char *bytecode; Upvariable* upvaluesEnd; };
char* bytecodeEnd;
c_string name, file, typeInfo;
uint_local_index_t nArgs;
uint_field_index_t iField;
union {
uint8_t flags;
struct {
bool vararg :1, fieldGetter :1, fieldSetter :1;
}; };
QV constants[];

static QFunction* create (QVM& vm, int nArgs, int nConstants, int nUpvalues, int bcSize);
QFunction (QVM& vm);
virtual bool gcVisit () final override;
virtual ~QFunction () = default;
virtual size_t getMemSize () override { return  bytecodeEnd - reinterpret_cast<char*>(this); }
};

#endif
