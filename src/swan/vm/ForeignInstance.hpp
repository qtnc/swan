#ifndef _____SWAN_FOREIGN_INSTANCE_HPP_____
#define _____SWAN_FOREIGN_INSTANCE_HPP_____
#include "Instance.hpp"

struct QForeignInstance: QSequence  {
char userData[];
QForeignInstance (QClass* type): QSequence(type) {}
static inline QForeignInstance* create (QClass* type, int nBytes) { return newVLS<QForeignInstance, char>(nBytes, type); }
virtual ~QForeignInstance ();
};
#endif
