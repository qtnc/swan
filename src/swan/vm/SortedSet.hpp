#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_SORTED_SET_HPP_____
#define _____SWAN_SORTED_SET_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include "Allocator.hpp"
#include <set>

void checkVersion(uint32_t,uint32_t);

struct QSortedSet: QSequence {
typedef std::multiset<QV, QVBinaryPredicate, trace_allocator<QV>> set_type; 
typedef set_type::iterator iterator;
set_type set;
QV sorter;
uint32_t version;
QSortedSet (struct QVM& vm, QV& sorter0);
inline void incrVersion () { version++; }
iterator find (const QV& key);
bool add (const QV& key, bool allowDuplicate = false);
~QSortedSet () = default;
bool gcVisit ();
inline bool copyInto (QFiber& f, CopyVisitor& out) { std::for_each(set.begin(), set.end(), std::ref(out)); return true; }
inline int getLength () { return set.size(); }
inline size_t getMemSize () { return sizeof(*this); }
};

struct QSortedSetIterator: QObject {
QSortedSet& set;
QSortedSet::iterator iterator;
uint32_t version;
bool forward;
QSortedSetIterator (QVM& vm, QSortedSet& s);
bool gcVisit ();
~QSortedSetIterator() = default;
inline void incrVersion () { version++; set.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, set.version); }
inline size_t getMemSize () { return sizeof(*this); }
};

#endif
#endif
