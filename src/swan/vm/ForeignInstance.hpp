#ifndef _____SWAN_FOREIGN_INSTANCE_HPP_____
#define _____SWAN_FOREIGN_INSTANCE_HPP_____
#include "Instance.hpp"

struct QForeignInstance: QSequence  {
char userData[];
QForeignInstance (QClass* type): QSequence(type) {}
static QForeignInstance* create (QClass* type, int nBytes);
~QForeignInstance ();
size_t getMemSize () ;
};
#endif
