#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_SORTED_SET_HPP_____
#define _____SWAN_SORTED_SET_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include <set>

struct QSortedSet: QSequence {
typedef std::multiset<QV, QVBinaryPredicate> set_type; 
typedef set_type::iterator iterator;
set_type set;
QV sorter;
QSortedSet (struct QVM& vm, QV& sorter0);
iterator find (const QV& key);
bool add (const QV& key, bool allowDuplicate = false);
virtual ~QSortedSet () = default;
virtual bool gcVisit () override;
};

struct QSortedSetIterator: QObject {
QSortedSet& set;
QSortedSet::iterator iterator;
QSortedSetIterator (QVM& vm, QSortedSet& s);
virtual bool gcVisit () override;
virtual ~QSortedSetIterator() = default;
};

#endif
#endif
