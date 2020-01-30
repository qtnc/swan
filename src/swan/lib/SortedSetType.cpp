#include "SwanLib.hpp"
#include "../vm/SortedSet.hpp"
#include "../vm/Fiber_inlines.hpp"
using namespace std;


static void setInstantiateFromItems (QFiber& f) {
int start=1, finish=f.getArgCount();
QV sorter =  QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (finish>=2) {
if (f.at(1).isCallable()) { sorter=f.at(1); start++; }
else if (f.at(-1).isCallable()) { sorter=f.at(-1); finish--; }
}
QSortedSet* set = f.vm.construct<QSortedSet>(f.vm, sorter);
f.returnValue(set);
for (int i=start, n=finish; i<n; i++) set->add(f.at(i));
}

static void setInstantiateFromSequences (QFiber& f) {
int start=1, finish=f.getArgCount();
QV sorter =  QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (finish>=2) {
if (f.at(1).isCallable()) { sorter=f.at(1); start++; }
else if (f.at(-1).isCallable()) { sorter=f.at(-1); finish--; }
}
QSortedSet* set = f.vm.construct<QSortedSet>(f.vm, sorter);
f.returnValue(set);
for (int i=start, l=finish; i<l; i++) {
f.getObject<QSequence>(i) .copyInto(f, *set);
}
}

static void setAdd (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
bool multi = false;
int n = f.getArgCount();
bool re = true;
if (f.isBool(-1)) { multi=f.getBool(-1); n--; }
for (int i=1; i<n; i++) re = set.add(f.at(i), multi) && re;
f.returnValue(re);
}

static void setIn (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = set.find(f.at(1));
f.returnValue(it!=set.set.end());
}

static void setUnion (QFiber& f) {
QSortedSet &set1 = f.getObject<QSortedSet>(0), &set2 = f.getObject<QSortedSet>(1);
QSortedSet* result = f.vm.construct<QSortedSet>(f.vm, set1.sorter);
f.returnValue(result);
set_union(set1.set.begin(), set1.set.end(), set2.set.begin(), set2.set.end(), inserter(result->set, result->set.begin()), set1.set.key_comp());
}

static void setIntersection (QFiber& f) {
QSortedSet &set1 = f.getObject<QSortedSet>(0), &set2 = f.getObject<QSortedSet>(1);
QSortedSet* result = f.vm.construct<QSortedSet>(f.vm, set1.sorter);
f.returnValue(result);
set_intersection(set1.set.begin(), set1.set.end(), set2.set.begin(), set2.set.end(), inserter(result->set, result->set.begin()), set1.set.key_comp());
}

static void setDifference (QFiber& f) {
QSortedSet &set1 = f.getObject<QSortedSet>(0), &set2 = f.getObject<QSortedSet>(1);
QSortedSet* result = f.vm.construct<QSortedSet>(f.vm, set1.sorter);
f.returnValue(result);
set_difference(set1.set.begin(), set1.set.end(), set2.set.begin(), set2.set.end(), inserter(result->set, result->set.begin()), set1.set.key_comp());
}

static void setSymetricDifference (QFiber& f) {
QSortedSet &set1 = f.getObject<QSortedSet>(0), &set2 = f.getObject<QSortedSet>(1);
QSortedSet* result = f.vm.construct<QSortedSet>(f.vm, set1.sorter);
f.returnValue(result);
set_symmetric_difference(set1.set.begin(), set1.set.end(), set2.set.begin(), set2.set.end(), inserter(result->set, result->set.begin()), set1.set.key_comp());
}

static void setRemove (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) {
auto it = set.find(f.at(i));
if (it==set.set.end()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(*it);
set.set.erase(it);
set.incrVersion();
}}}

static void setEquals (QFiber& f) {
QSortedSet &s1 = f.getObject<QSortedSet>(0), &s2 = f.getObject<QSortedSet>(1);
if (s1.set.size() != s2.set.size() ) { f.returnValue(false); return; }
bool re = true;
for (auto& x: s1.set) {
if (s2.set.end() == s2.find(x)) { re=false; break; }
}
f.returnValue(re);
}

static void setIterator (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = f.vm.construct<QSortedSetIterator>(f.vm, set);
f.returnValue(it);
}

static void setIteratorNext (QFiber& f) {
QSortedSetIterator& li = f.getObject<QSortedSetIterator>(0);
li.checkVersion();
if (li.iterator==li.set.set.end()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*li.iterator++);
li.forward=true;
}

static void setIteratorPrevious (QFiber& f) {
QSortedSetIterator& li = f.getObject<QSortedSetIterator>(0);
li.checkVersion();
if (li.iterator==li.set.set.begin()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*--li.iterator);
li.forward=false;
}

static void setIteratorRemove (QFiber& f) {
QSortedSetIterator& mi = f.getObject<QSortedSetIterator>(0);
mi.checkVersion();
if (mi.forward) --mi.iterator;
f.returnValue(*mi.iterator);
mi.set.set.erase(mi.iterator++);
mi.set.incrVersion();
}

static void setLowerBound (QFiber& f) {
QSortedSet &set = f.getObject<QSortedSet>(0);
auto it = set.set.lower_bound(f.at(1));
f.returnValue(it==set.set.end()? QV::UNDEFINED : *it);
}

static void setUpperBound (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = set.set.upper_bound(f.at(1));
f.returnValue(it==set.set.end()? QV::UNDEFINED : *it);
}

static void setFirst (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = set.set.begin();
f.returnValue(it!=set.set.end()? *it : QV::UNDEFINED);
}

static void setLast (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = set.set.end();
f.returnValue(set.set.size()? *--it : QV::UNDEFINED);
}

static void setShift (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = set.set.begin();
if (it==set.set.end()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(*it);
set.set.erase(it);
set.incrVersion();
}}

static void setPop (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
if (set.set.size()) {
auto it = set.set.end();
f.returnValue(*--it);
set.set.erase(it);
set.incrVersion();
}
else f.returnValue(QV::UNDEFINED);
}

static void setToString (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
string re = "<";
set.join(f, ", ", re);
re += ">";
f.returnValue(re);
}



void QVM::initSortedSetType () {
sortedSetClass
->copyParentMethods()
->bind("&", setIntersection)
->bind("|", setUnion)
->bind("-", setDifference)
->bind("^", setSymetricDifference)
->bind("iterator", setIterator)
->bind("toString", setToString)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QSortedSet>(0).set.size())); })
BIND_L(clear, { f.getObject<QSortedSet>(0).set.clear(); })
->bind("add", setAdd)
->bind("remove", setRemove)
->bind("lower", setLowerBound)
->bind("upper", setUpperBound)
->bind("pop", setPop)
->bind("shift", setShift)
->bind("first", setFirst)
->bind("last", setLast)
->bind("in", setIn)
->bind("==", setEquals)
;

sortedSetIteratorClass
->copyParentMethods()
->bind("next", setIteratorNext)
->bind("previous", setIteratorPrevious)
->bind("remove", setIteratorRemove)
;

sortedSetClass ->type
->copyParentMethods()
->bind("()", setInstantiateFromSequences)
->bind("of", setInstantiateFromItems)
;

//println("sizeof(QSortedSet)=%d", sizeof(QSortedSet));
}

