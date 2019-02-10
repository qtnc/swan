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
T intersection, union_;
unordered_set_union(set1, set2, union_);
unordered_set_intersection(set1, set2, intersection);
unordered_set_difference(union_, intersection, result);
}


static void setInstantiate (QFiber& f) {
int n = f.getArgCount() -1;
QSet* set = new QSet(f.vm);
if (n>0) set->set.insert(&f.at(1), &f.at(1) +n);
f.returnValue(set);
}

static void setFromSequence (QFiber& f) {
QSet* set = new QSet(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i).insertIntoSet(f, *set);
}
f.returnValue(set);
}

static void setAdd (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
int n = f.getArgCount() -1;
if (n>0) set.set.insert(&f.at(1), (&f.at(1))+n);
}

static void setIn (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
auto it = set.set.find(f.at(1));
f.returnValue(it!=set.set.end());
}

static void setUnion (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = new QSet(f.vm);
unordered_set_union(set1.set, set2.set, result->set);
f.returnValue(result);
}

static void setIntersection (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = new QSet(f.vm);
unordered_set_intersection(set1.set, set2.set, result->set);
f.returnValue(result);
}

static void setDifference (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = new QSet(f.vm);
unordered_set_difference(set1.set, set2.set, result->set);
f.returnValue(result);
}

static void setSymetricDifference (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = new QSet(f.vm);
unordered_set_symetric_difference(set1.set, set2.set, result->set);
f.returnValue(result);
}

static void setRemove (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) {
auto it = set.set.find(f.at(i));
if (it==set.set.end()) f.returnValue(QV());
else {
f.returnValue(*it);
set.set.erase(it);
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

static void setIterate (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
if (f.isNull(1)) {
f.returnValue(new QSetIterator(f.vm, set));
}
else {
QSetIterator& mi = f.getObject<QSetIterator>(1);
bool cont = mi.iterator != set.set.end();
f.returnValue( cont? f.at(1) : QV());
}}

static void setIteratorValue (QFiber& f) {
QSetIterator& mi = f.getObject<QSetIterator>(1);
QV val = *mi.iterator++;
f.returnValue(val);
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
BIND_F(&, setIntersection)
BIND_F(|, setUnion)
BIND_F(-, setDifference)
BIND_F(^, setSymetricDifference)
BIND_F(iteratorValue, setIteratorValue)
BIND_F(iterate, setIterate)
BIND_F(toString, setToString)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QSet>(0).set.size())); })
BIND_L(clear, { f.getObject<QSet>(0).set.clear(); })
BIND_F(add, setAdd)
BIND_F(remove, setRemove)
BIND_F(in, setIn)
BIND_F(==, setEquals)
;


setMetaClass
->copyParentMethods()
BIND_F( (), setInstantiate)
BIND_F(of, setFromSequence)
;
}

