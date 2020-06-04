#ifndef _____SWAN_FUNCTIONFLAGS_HPP_____
#define _____SWAN_FUNCTIONFLAGS_HPP_____
#include<cstdint>

union FunctionFlags {
uint8_t value;
struct {
bool vararg: 1;
bool pure: 1;
bool final: 1;
bool overridden: 1;
}; 
inline FunctionFlags (): value(0) {}
};

#endif
