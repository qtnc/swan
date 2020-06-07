#ifndef _____SWAN_FUNCTION_HPP_____
#define _____SWAN_FUNCTION_HPP_____
#include "../../include/bitfield.hpp"
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

bitfield(FunctionFlag, uint8_t){
None = 0,
Vararg = 1,
Pure = 2,
Final = 4,
Overridden = 8,
Accessor = 0x10,
Static = 0x20,
};

struct QFunction: QObject {
union { Upvariable* upvalues; QV* constantsEnd; };
union { char *bytecode; Upvariable* upvaluesEnd; };
union { char* bytecodeEnd; DebugItem* debugItems; };
union { DebugItem* debugItemsEnd; };
c_string name, file, typeInfo;
uint_local_index_t nArgs;
uint_field_index_t fieldIndex;
bitmask<FunctionFlag> flags;
QV constants[];

static QFunction* create (QVM& vm, int nArgs, int nConstants, int nUpvalues, int bcSize, int nDebugItems = 0);
QFunction (QVM& vm);
bool gcVisit ();
~QFunction () = default;
inline size_t getMemSize ()  { return  bytecodeEnd - reinterpret_cast<char*>(this); }
void disasm (std::ostream& out) const;
};

#endif
