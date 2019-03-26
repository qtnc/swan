#ifndef _____SWAN_SEQUENCE_HPP_____
#define _____SWAN_SEQUENCE_HPP_____
#include "Core.hpp"
#include "Object.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include<vector>
#include<string>

struct QSequence: QObject {
QSequence (QClass* c): QObject(c) {}
virtual void copyInto (QFiber& f, QSequence& seq);
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1);
virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1);
virtual void join (struct QFiber& f, const std::string& delim, std::string& out);
};

#endif
