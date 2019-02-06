#ifndef _____SWAN_FUNCTION_HPP_____
#define _____SWAN_FUNCTION_HPP_____
#include "Object.hpp"
#include "Value.hpp"
#include<string>
#include<vector>

struct QFunction: QObject {
struct Upvalue {
int slot;
bool upperUpvalue;
};
std::string bytecode, name, file;
std::vector<Upvalue> upvalues;
std::vector<QV> constants;
uint8_t nArgs;
bool vararg;

QFunction (QVM& vm);
virtual bool gcVisit () final override;
virtual ~QFunction () = default;
};

#endif
