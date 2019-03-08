#ifndef NO_OPTIONAL_COLLECTIONS
#include "SortedSet.hpp"
#include "HasherAndEqualler.hpp"
#include "VM.hpp"
using namespace std;

QSortedSetIterator::QSortedSetIterator (QVM& vm, QSortedSet& m): 
QObject(vm.objectClass), set(m), iterator(m.set.begin()) 
{}

QSortedSet::QSortedSet (QVM& vm, QV& sorter0): 
QSequence(vm.sortedSetClass), 
set(QVBinaryPredicate(vm, sorter0), trace_allocator<QV>(vm)), 
sorter(sorter0) 
{}

QSortedSet::iterator QSortedSet::find (const QV& key) {
auto range = set.equal_range(key);
if (range.first==range.second) return set.end();
QVEqualler eq(type->vm);
auto it = find_if(range.first, range.second, [&](const auto& i){ return eq(i, key); });
if (it!=range.second) return it;
else return set.end();
}

bool QSortedSet::add (const QV& key, bool allowDuplicate) {
if (allowDuplicate) {
set.insert(key);
return true;
}
auto it = find(key);
if (it!=set.end()) return false;
set.insert(key);
return true;
}

#endif
