#ifndef _____SWAN_FUNCTIONFLAGS_HPP_____
#define _____SWAN_FUNCTIONFLAGS_HPP_____
#include "Constants.hpp"
#include<cstdint>

struct FunctionFlags {
uint_field_index_t fieldIndex;
union {
uint8_t value;
struct {
bool vararg: 1;
bool pure: 1;
bool final: 1;
bool overridden: 1;
bool accessor: 1;
}; };
inline FunctionFlags (): fieldIndex(0), value(0) {}
};

#endif
