#ifndef _____SWAN_OPCODE_INFO_HPP_____
#define _____SWAN_OPCODE_INFO_HPP_____

struct OpCodeInfo {
int stackEffect, nArgs, argFormat;
};

enum QOpCode {
#define OP(name, stackEffect, nArgs, argFormat) OP_##name
#include "OpCodes.hpp"
#undef OP
};

extern OpCodeInfo OPCODE_INFO[];


#endif
