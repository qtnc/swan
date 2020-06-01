#ifndef _____SWAN_FUNCTIONFLAGS_HPP_____
#define _____SWAN_FUNCTIONFLAGS_HPP_____

struct FunctionFlags {
uint_field_index_t iField;
union {
uint8_t flags;
struct {
bool vararg: 1,
fieldGetter: 1, fieldSetter: 1, 
pure: 1, final: 1, overridden: 1;
}; };
inline FunctionFlags (): iField(0), flags(0) {}
};

#endif
