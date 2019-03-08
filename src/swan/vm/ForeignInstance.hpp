#ifndef _____SWAN_FOREIGN_INSTANCE_HPP_____
#define _____SWAN_FOREIGN_INSTANCE_HPP_____
#include "Instance.hpp"

struct QForeignInstance: QSequence  {
char userData[];
QForeignInstance (QClass* type): QSequence(type) {}
static QForeignInstance* create (QClass* type, int nBytes);
virtual ~QForeignInstance ();
virtual size_t getMemSize () override ;
};
#endif
