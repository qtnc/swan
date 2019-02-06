#ifndef _____TOKENS_TYPES_HPP_____
#define _____TOKENS_TYPES_HPP_____
#include "../vm/Value.hpp"

enum QTokenType {
#define TOKEN(name) T_##name
#include "TokenTypes.hpp"
#undef TOKEN
};

struct QToken {
QTokenType type;
const char *start;
size_t length;
QV value;
};

#endif
