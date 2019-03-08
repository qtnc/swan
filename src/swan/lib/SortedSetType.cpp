#include "SwanLib.hpp"
#include "../vm/SortedSet.hpp"
#include "../vm/Fiber_inlines.hpp"
using namespace std;


static void setInstantiate (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (!sorter.isCallable()) f.runtimeError("Sorter must be callable");
QSortedSet* set = f.vm.construct<QSortedSet>(f.vm, sorter);
f.returnValue(set);
for (int i=2, n=f.getArgCount(); i<n; i++) set->add(f.at(i));
}

static void setFromSequence (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (!sorter.isCallable()) f.runtimeError("Sorter must be callable");
QSortedSet* set = f.vm.construct<QSortedSet>(f.vm, sorter);
f.returnValue(set);
for (int i=2, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i) .copyInto(f, *set);
}
}

static void setAdd (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
bool multi = false;
int n = f.getArgCount();
if (f.isBool(-1)) { multi=f.getBool(-1); n--; }
for (int i=1; i<n; i++) set.add(f.at(i), multi);
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
if (it==set.set.end()) f.returnValue(QV());
else {
f.returnValue(*it);
set.set.erase(it);
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

static void setIterate (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
if (f.isNull(1)) {
f.returnValue(f.vm.construct<QSortedSetIterator>(f.vm, set));
}
else {
QSortedSetIterator& mi = f.getObject<QSortedSetIterator>(1);
bool cont = mi.iterator != set.set.end();
f.returnValue( cont? f.at(1) : QV());
}}

static void setIteratorValue (QFiber& f) {
QSortedSetIterator& mi = f.getObject<QSortedSetIterator>(1);
QV val = *mi.iterator++;
f.returnValue(val);
}

static void setLowerBound (QFiber& f) {
QSortedSet &set = f.getObject<QSortedSet>(0);
auto it = set.set.lower_bound(f.at(1));
f.returnValue(it==set.set.end()? QV() : *it);
}

static void setUpperBound (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = set.set.upper_bound(f.at(1));
f.returnValue(it==set.set.end()? QV() : *it);
}

static void setFirst (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = set.set.begin();
f.returnValue(it!=set.set.end()? *it : QV());
}

static void setLast (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = set.set.end();
f.returnValue(set.set.size()? *--it : QV());
}

static void setShift (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
auto it = set.set.begin();
if (it==set.set.end()) f.returnValue(QV());
else {
f.returnValue(*it);
set.set.erase(it);
}}

static void setPop (QFiber& f) {
QSortedSet& set = f.getObject<QSortedSet>(0);
if (set.set.size()) {
auto it = set.set.end();
f.returnValue(*--it);
set.set.erase(it);
}
else f.returnValue(QV());
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
BIND_F(&, setIntersection)
BIND_F(|, setUnion)
BIND_F(-, setDifference)
BIND_F(^, setSymetricDifference)
BIND_F(iteratorValue, setIteratorValue)
BIND_F(iterate, setIterate)
BIND_F(toString, setToString)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QSortedSet>(0).set.size())); })
BIND_L(clear, { f.getObject<QSortedSet>(0).set.clear(); })
BIND_F(add, setAdd)
BIND_F(remove, setRemove)
BIND_F(lower, setLowerBound)
BIND_F(upper, setUpperBound)
BIND_F(pop, setPop)
BIND_F(shift, setShift)
BIND_F(first, setFirst)
BIND_F(last, setLast)
BIND_F(in, setIn)
BIND_F(==, setEquals)
;


sortedSetMetaClass
->copyParentMethods()
BIND_F( (), setInstantiate)
BIND_F(of, setFromSequence)
;

println("sizeof(QSortedSet)=%d", sizeof(QSortedSet));
}

