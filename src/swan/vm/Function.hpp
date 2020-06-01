#ifndef _____SWAN_FUNCTION_HPP_____
#define _____SWAN_FUNCTION_HPP_____
#include "FunctionFlags.hpp"
#include "Object.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include "CString.hpp"
#include<string>
#include<vector>

struct Upvariable {
uint_local_index_t slot;
bool upperUpvalue :1;
};

struct DebugItem {
int32_t offset;
int16_t line;
};

struct QFunction: QObject {
union { Upvariable* upvalues; QV* constantsEnd; };
union { char *bytecode; Upvariable* upvaluesEnd; };
union { char* bytecodeEnd; DebugItem* debugItems; };
union { DebugItem* debugItemsEnd; };
c_string name, file, typeInfo;
uint_local_index_t nArgs;
FunctionFlags flags;
QV constants[];

static QFunction* create (QVM& vm, int nArgs, int nConstants, int nUpvalues, int bcSize, int nDebugItems = 0);
QFunction (QVM& vm);
bool gcVisit ();
~QFunction () = default;
inline size_t getMemSize ()  { return  bytecodeEnd - reinterpret_cast<char*>(this); }
void disasm (std::ostream& out) const;
};

#endif
