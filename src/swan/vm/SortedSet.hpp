#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_SORTED_SET_HPP_____
#define _____SWAN_SORTED_SET_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include "Allocator.hpp"
#include <set>

struct QSortedSet: QSequence {
typedef std::multiset<QV, QVBinaryPredicate, trace_allocator<QV>> set_type; 
typedef set_type::iterator iterator;
set_type set;
QV sorter;
QSortedSet (struct QVM& vm, QV& sorter0);
iterator find (const QV& key);
bool add (const QV& key, bool allowDuplicate = false);
virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { set.insert(v.begin(), v.end()); }
virtual ~QSortedSet () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QSortedSetIterator: QObject {
QSortedSet& set;
QSortedSet::iterator iterator;
QSortedSetIterator (QVM& vm, QSortedSet& s);
virtual bool gcVisit () override;
virtual ~QSortedSetIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
#endif
