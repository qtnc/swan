#ifndef _____SWAN_INSTANCE_HPP_____
#define _____SWAN_INSTANCE_HPP_____
#include "Iterable.hpp"

struct QInstance: QSequence {
QV fields[];
QInstance (QClass* type): QSequence(type) {}
static QInstance* create (QClass* type, int nFields);
~QInstance () = default;
bool gcVisit ();
size_t getMemSize ();
};

#endif
