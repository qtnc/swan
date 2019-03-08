#ifndef _____SWAN_OBJECT_HPP_____
#define _____SWAN_OBJECT_HPP_____
#include "Core.hpp"

struct QObject {
QClass* type;
QObject* next;
QObject (QClass* tp);
virtual bool gcVisit ();
virtual size_t getMemSize () = 0;
virtual ~QObject() = default;
};

#endif
