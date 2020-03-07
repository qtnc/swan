#ifndef _____SWAN_OPCODE_INFO_HPP_____
#define _____SWAN_OPCODE_INFO_HPP_____

struct OpCodeInfo {
int8_t stackEffect;
uint8_t szArgs;
uint16_t argFormat;
};

enum QOpCode {
#define OP(name, stackEffect, szArgs, argFormat) OP_##name
#include "OpCodes.hpp"
#undef OP
};

extern OpCodeInfo OPCODE_INFO[];


#endif
