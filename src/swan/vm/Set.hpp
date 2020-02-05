#ifndef _____SWAN_SET_HPP_____
#define _____SWAN_SET_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include "VM.hpp"
#include "HasherAndEqualler.hpp"
#include<unordered_set>

void checkVersion(uint32_t,uint32_t);

struct QSet: QSequence {
typedef std::unordered_set<QV, QVHasher, QVEqualler, trace_allocator<QV>> set_type;
typedef set_type::iterator iterator;
set_type set;
uint32_t version;
QSet (QVM& vm);
inline void incrVersion () { version++; }
inline bool copyInto (QFiber& f, CopyVisitor& out) { std::for_each(set.begin(), set.end(), std::ref(out)); return true; }
inline int getLength () { return set.size(); }
bool join (QFiber& f, const std::string& delim, std::string& out);
~QSet () = default;
bool gcVisit ();
inline size_t getMemSize () { return sizeof(*this); }
};

struct QSetIterator: QObject {
QSet& set;
QSet::iterator iterator;
uint32_t version;
QSetIterator (QVM& vm, QSet& m): QObject(vm.setIteratorClass), set(m), iterator(m.set.begin()), version(m.version)  {}
bool gcVisit ();
~QSetIterator() = default;
inline void incrVersion () { version++; set.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, set.version); }
inline size_t getMemSize () { return sizeof(*this); }
};

#endif
