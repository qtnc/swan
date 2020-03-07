#ifndef _____SWAN_OBJECT_HPP_____
#define _____SWAN_OBJECT_HPP_____
#include "Constants.hpp"
#include<string>

struct CopyVisitor {
virtual void operator() (const union QV&) = 0;
};

template<class T> struct CopyVisitorIterator: CopyVisitor {
T out;
inline CopyVisitorIterator (T x): out(x) {}
void operator() (const QV& x) override { *out++ = x; }
};

template<class T> CopyVisitorIterator<T> copyVisitor (T x) {
return CopyVisitorIterator<T>(x);
}

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
bool gcVisit (); 
void* gcOrigin ();
inline bool join (struct QFiber& f, const std::string& delim, std::string& out) { return false; }
inline bool copyInto (struct QFiber& f, CopyVisitor& out) { return false; }
inline int getLength () { return -2; }
~QObject() = default;
inline size_t getMemSize () { return sizeof(*this); }
};

#endif
