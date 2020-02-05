#ifndef _____SWAN_SEQUENCE_HPP_____
#define _____SWAN_SEQUENCE_HPP_____
#include "Core.hpp"
#include "Object.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include<vector>
#include<string>

struct QSequence: QObject {
inline QSequence (QClass* c): QObject(c) {}
inline int getLength () { return -1; }
bool copyInto (QFiber& f, CopyVisitor& out);
bool join (struct QFiber& f, const std::string& delim, std::string& out);
};

#endif
