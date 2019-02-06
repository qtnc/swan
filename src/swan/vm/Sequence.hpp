#ifndef _____SWAN_SEQUENCE_HPP_____
#define _____SWAN_SEQUENCE_HPP_____
#include "Core.hpp"
#include "Object.hpp"
#include "Value.hpp"
#include<vector>
#include<string>

struct QSequence: QObject {
QSequence (QClass* c): QObject(c) {}
virtual void insertIntoVector (struct QFiber& f, std::vector<QV>& list, int start);
virtual void insertIntoSet (struct QFiber& f, struct QSet&);
virtual void join (struct QFiber& f, const std::string& delim, std::string& out);
};

#endif
