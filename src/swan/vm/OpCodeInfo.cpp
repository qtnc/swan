#include "Constants.hpp"
#include "OpCodeInfo.hpp" 

OpCodeInfo OPCODE_INFO[] = {
#define OP(name, stackEffect, szArgs, argFormat) { stackEffect, szArgs, argFormat }
#include "OpCodes.hpp"
#undef OP
};

