#include "../../include/cpprintf.hpp"
#include "SwanLib.hpp"
#include "../vm/Set.hpp"
using namespace std;

template<class T> void unordered_set_union (const T& set1, const T& set2, T& result) {
for (auto& val: set1) result.insert(val);
for (auto& val: set2) result.insert(val);
}

template<class T> void unordered_set_intersection (const T& set1, const T& set2, T& result) {
for (auto& val: set1) if (set2.find(val)!=set2.end()) result.insert(val);
}

template<class T> void unordered_set_difference  (const T& set1, const T& set2, T& result) {
for (auto& val: set1) if (set2.find(val)==set2.end()) result.insert(val);
}

template<class T> void unordered_set_symetric_difference  (const T& set1, const T& set2, T& result) {
T intersection(set1.bucket_count(), set1.hash_function(), set1.key_eq(), set1.get_allocator()), union_(set1.bucket_count(), set1.hash_function(), set1.key_eq(), set1.get_allocator());
unordered_set_union(set1, set2, union_);
unordered_set_intersection(set1, set2, intersection);
unordered_set_difference(union_, intersection, result);
}


static void setInstantiateFromItems (QFiber& f) {
int n = f.getArgCount() -1;
QSet* set = f.vm.construct<QSet>(f.vm);
f.returnValue(set);
if (n>0) set->set.insert(&f.at(1), &f.at(1) +n);
}

static void setInstantiateFromSequences (QFiber& f) {
QSet* set = f.vm.construct<QSet>(f.vm);
f.returnValue(set);
for (int i=1, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i) .copyInto(f, *set);
}
}

static void setAdd (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
int n = f.getArgCount() -1;
auto s = set.set.size();
if (n>0) set.set.insert(&f.at(1), (&f.at(1))+n);
f.returnValue(set.set.size() == n+s);
set.incrVersion();
}

static void setIn (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
auto it = set.set.find(f.at(1));
f.returnValue(it!=set.set.end());
}

static void setUnion (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = f.vm.construct<QSet>(f.vm);
f.returnValue(result);
unordered_set_union(set1.set, set2.set, result->set);
}

static void setIntersection (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = f.vm.construct<QSet>(f.vm);
f.returnValue(result);
unordered_set_intersection(set1.set, set2.set, result->set);
}

static void setDifference (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = f.vm.construct<QSet>(f.vm);
f.returnValue(result);
unordered_set_difference(set1.set, set2.set, result->set);
}

static void setSymetricDifference (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = f.vm.construct<QSet>(f.vm);
f.returnValue(result);
unordered_set_symetric_difference(set1.set, set2.set, result->set);
}

static void setRemove (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) {
auto it = set.set.find(f.at(i));
if (it==set.set.end()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(*it);
set.set.erase(it);
set.incrVersion();
}}}

static void setEquals (QFiber& f) {
QSet &s1 = f.getObject<QSet>(0), &s2 = f.getObject<QSet>(1);
if (s1.set.size() != s2.set.size() ) { f.returnValue(false); return; }
bool re = true;
for (auto& x: s1.set) {
if (s2.set.end() == s2.set.find(x)) { re=false; break; }
}
f.returnValue(re);
}

static void setLength (QFiber& f) {
auto& set = f.getObject<QSet>(0);
f.returnValue(static_cast<double>(set.set.size()));
}

static void setClear (QFiber& f) {
auto& set = f.getObject<QSet>(0);
set.set.clear();
}

static void setReserve (QFiber& f) {
auto& set = f.getObject<QSet>(0);
set.set.reserve(f.getNum(1));
}

static void setIterator (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
auto it = f.vm.construct<QSetIterator>(f.vm, set);
f.returnValue(it);
}

static void setIteratorNext (QFiber& f) {
QSetIterator& li = f.getObject<QSetIterator>(0);
li.checkVersion();
if (li.iterator==li.set.set.end()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*li.iterator++);
}

static void setToString (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
string re = "<";
set.join(f, ", ", re);
re += ">";
f.returnValue(re);
}



void QVM::initSetType () {
setClass
->copyParentMethods()
->bind("&", setIntersection)
->bind("|", setUnion)
->bind("-", setDifference)
->bind("^", setSymetricDifference)
->bind("iterator", setIterator)
->bind("toString", setToString)
->bind("length", setLength)
->bind("clear", setClear)
->bind("add", setAdd)
->bind("remove", setRemove)
->bind("reserve", setReserve)
->bind("in", setIn)
->bind("==", setEquals)
;

setIteratorClass
->copyParentMethods()
->bind("next", setIteratorNext)
;

setClass ->type
->copyParentMethods()
->bind("()", setInstantiateFromSequences)
->bind("of", setInstantiateFromItems)
;

//println("sizeof(QSet)=%d", sizeof(QSet));
}

