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
virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { set.insert(v.begin(), v.end()); incrVersion(); }
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { 
auto it = start<0? v.end() +start +1 : v.begin() + start;
v.insert(it, set.begin(), set.end());
}
virtual ~QSortedSet () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QSortedSetIterator: QObject {
QSortedSet& set;
QSortedSet::iterator iterator;
uint32_t version;
bool forward;
QSortedSetIterator (QVM& vm, QSortedSet& s);
virtual bool gcVisit () override;
virtual ~QSortedSetIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
inline void incrVersion () { version++; set.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, set.version); }
};

#endif
#endif
