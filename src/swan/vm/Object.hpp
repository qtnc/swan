#ifndef _____SWAN_OBJECT_HPP_____
#define _____SWAN_OBJECT_HPP_____
#include "Core.hpp"

struct QObject {
QClass* type;
QObject* next;
QObject (QClass* tp);

inline bool gcMarked (int n=1) { return reinterpret_cast<uintptr_t>(next) &n; }
inline bool gcMark (int n=1) {
if (gcMarked(n)) return true;
next = reinterpret_cast<QObject*>( reinterpret_cast<uintptr_t>(next) |n);
return false;
}
inline void gcUnmark (int n=1) { next = reinterpret_cast<QObject*>( reinterpret_cast<uintptr_t>(next) &~n); }
inline QObject* gcNext () { return reinterpret_cast<QObject*>(reinterpret_cast<uintptr_t>(next) &~3); }
inline void gcNext (QObject* p) { next = reinterpret_cast<QObject*>(reinterpret_cast<uintptr_t>(p) | (reinterpret_cast<uintptr_t>(next)&3)); }
virtual bool gcVisit ();
virtual size_t getMemSize () = 0;

virtual ~QObject() = default;
};

#endif
