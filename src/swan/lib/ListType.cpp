#include "../../include/cpprintf.hpp"
#include "SwanLib.hpp"
#include "../vm/List.hpp"
using namespace std;

static void listInstantiate (QFiber& f) {
int n = f.getArgCount() -1;
QList* list = f.vm.construct<QList>(f.vm);
f.returnValue(list);
if (n>0) list->data.insert(list->data.end(), &f.at(1), &f.at(1) +n);
}

static void listIterator (QFiber& f) {
QList& list = f.getObject<QList>(0);
int index = f.getOptionalNum(1, 0);
if (index<0) index += list.data.size() +1;
auto it = f.vm.construct<QListIterator>(f.vm, list);
if (index>0) std::advance(it->iterator, index);
f.returnValue(it);
}

static void listIteratorNext (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
if (li.iterator==li.list.data.end()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*li.iterator++);
li.forward=true;
}

static void listIteratorPrevious (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
if (li.iterator==li.list.data.begin()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*--li.iterator);
li.forward=false;
}

static void listIteratorRemove (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
if (li.forward) --li.iterator;
f.returnValue(*li.iterator);
li.iterator = li.list.data.erase(li.iterator);
}

static void listIteratorInsert (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
QV value = f.at(1);
li.iterator = li.list.data.insert(li.iterator, value);
if (li.forward) ++li.iterator;
}

static void listSubscript (QFiber& f) {
QList& list = f.getObject<QList>(0);
int length = list.data.size();
if (f.isNum(1)) {
int i = f.getNum(1);
if (i<0) i+=length;
f.returnValue(i>=0 && i<length? list.data.at(i) : QV::UNDEFINED);
}
else if (f.isRange(1)) {
int start, end;
f.getRange(1).makeBounds(length, start, end);
QList* newList = f.vm.construct<QList>(f.vm);
f.returnValue(newList);
if (end-start>0) newList->data.insert(newList->data.end(), list.data.begin()+start, list.data.begin()+end);
}
else f.returnValue(QV::UNDEFINED);
}

static void listSubscriptSetter (QFiber& f) {
QList& list = f.getObject<QList>(0);
int length = list.data.size();
if (f.isNum(1)) {
int i = f.getNum(1);
if (i<0) i+=length;
f.returnValue(list.data.at(i) = f.at(2));
}
else if (f.isRange(1)) {
int start, end;
f.getRange(1).makeBounds(length, start, end);
list.data.erase(list.data.begin()+start, list.data.begin()+end);
f.getObject<QSequence>(2) .copyInto(f, list.data, start);
f.returnValue(f.at(2));
}
else f.returnValue(QV::UNDEFINED);
}

static void listPush (QFiber& f) {
QList& list = f.getObject<QList>(0);
int n = f.getArgCount() -1;
if (n>0) list.data.insert(list.data.end(), &f.at(1), (&f.at(1))+n);
}

static void listPop (QFiber& f) {
QList& list = f.getObject<QList>(0);
if (list.data.empty()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(list.data.back());
list.data.pop_back();
}}

static void listInsert (QFiber& f) {
QList& list = f.getObject<QList>(0);
int n = f.getNum(1), count = f.getArgCount() -2;
auto pos = n>=0? list.data.begin()+n : list.data.end()+n;
list.data.insert(pos, &f.at(2), (&f.at(2))+count);
}

static void listRemoveAt (QFiber& f) {
QList& list = f.getObject<QList>(0);
for (int i=1, l=f.getArgCount(); i<l; i++) {
if (f.isNum(i)) {
int n = f.getNum(i);
auto pos = n>=0? list.data.begin()+n : list.data.end()+n;
list.data.erase(pos);
}
else if (f.isRange(i)) {
int start, end;
f.getRange(i).makeBounds(list.data.size(), start, end);
list.data.erase(list.data.begin()+start, list.data.begin()+end);
}
}}

static void listRemove (QFiber& f) {
QList& list = f.getObject<QList>(0);
QVEqualler eq(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
QV& toRemove = f.at(i);
auto it = find_if(list.data.begin(), list.data.end(), [&](const QV& v){ return eq(v, toRemove); });
if (it!=list.data.end()) {
f.returnValue(*it);
list.data.erase(it);
}
else f.returnValue(QV::UNDEFINED);
}}

static void listRemoveIf (QFiber& f) {
QList& list = f.getObject<QList>(0);
for (int i=1, l=f.getArgCount(); i<l; i++) {
QVUnaryPredicate pred(f.vm, f.at(i));
auto it = remove_if(list.data.begin(), list.data.end(), pred);
list.data.erase(it, list.data.end());
}}

static void listIndexOf (QFiber& f) {
QList& list = f.getObject<QList>(0);
QV& needle = f.at(1);
int start = f.getOptionalNum(2, 0);
QVEqualler eq(f.vm);
auto end = list.data.end(), begin = start>=0? list.data.begin()+start : list.data.end()+start,
re = find_if(begin, end, [&](const QV& v){ return eq(v, needle); });
f.returnValue(re==end? -1.0 : static_cast<double>(re-list.data.begin()));
}

static void listLastIndexOf (QFiber& f) {
QList& list = f.getObject<QList>(0);
QV& needle = f.at(1);
int start = f.getOptionalNum(2, list.data.size());
auto begin = list.data.begin(), end = start>=0? list.data.begin()+start : list.data.end()+start,
re = find_end(begin, end, &needle, (&needle)+1, QVEqualler(f.vm));
f.returnValue(re==end? -1.0 : static_cast<double>(re-list.data.begin()));
}

static void listSort (QFiber& f) {
QList& list = f.getObject<QList>(0);
if (f.getArgCount()>=2) stable_sort(list.data.begin(), list.data.end(), QVBinaryPredicate(f.vm, f.at(1)));
else stable_sort(list.data.begin(), list.data.end(), QVLess(f.vm));
}

static void listReverse (QFiber& f) {
QList& list = f.getObject<QList>(0);
reverse(list.data.begin(), list.data.end());
}

static void listRotate (QFiber& f) {
QList& list = f.getObject<QList>(0);
int offset = f.getNum(1);
auto middle = offset>=0? list.data.begin()+offset : list.data.end()+offset;
rotate(list.data.begin(), middle, list.data.end());
}

static void listTimes (QFiber& f) {
QList *list = f.at(0).asObject<QList>(), *newList = f.vm.construct<QList>(f.vm);
int times = f.getNum(1);
f.returnValue(newList);
if (times>0) for (int i=0; i<times; i++) newList->data.insert(newList->data.end(), list->data.begin(), list->data.end());
}

static void listLowerBound (QFiber& f) {
QList& list = f.getObject<QList>(0);
QV value = f.at(1);
auto it = list.data.end();
if (f.getArgCount()>2) it = lower_bound(list.data.begin(), list.data.end(), value, QVBinaryPredicate(f.vm, f.at(2)));
else it = lower_bound(list.data.begin(), list.data.end(), value, QVLess(f.vm));
f.returnValue(static_cast<double>(it - list.data.begin() ));
}

static void listUpperBound (QFiber& f) {
QList& list = f.getObject<QList>(0);
QV value = f.at(1);
auto it = list.data.end();
if (f.getArgCount()>2) it = upper_bound(list.data.begin(), list.data.end(), value, QVBinaryPredicate(f.vm, f.at(2)));
else it = upper_bound(list.data.begin(), list.data.end(), value, QVLess(f.vm));
f.returnValue(static_cast<double>(it - list.data.begin() ));
}

static void listEquals (QFiber& f) {
QList &l1 = f.getObject<QList>(0), &l2 = f.getObject<QList>(1);
if (l1.data.size() != l2.data.size() ) { f.returnValue(false); return; }
int eqSymbol = f.vm.findMethodSymbol("==");
bool re = true;
for (size_t i=0, n=l1.data.size(); re && i<n; i++) {
f.pushCppCallFrame();
f.push(l1.data[i]);
f.push(l2.data[i]);
f.callSymbol(eqSymbol, 2);
re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
}
f.returnValue(re);
}

static void listFromSequence (QFiber& f) {
QList* list = f.vm.construct<QList>(f.vm);
f.returnValue(list);
for (int i=1, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i) .copyInto(f, list->data);
}
}

static void listToString (QFiber& f) {
QList& list = f.getObject<QList>(0);
string re = "[";
list.join(f, ", ", re);
re += "]";
f.returnValue(re);
}


void QVM::initListType () {
listClass
->copyParentMethods()
BIND_F( [], listSubscript)
BIND_F( []=, listSubscriptSetter)
BIND_F(iterator, listIterator)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QList>(0).data.size())); })
BIND_F(toString, listToString)
BIND_L(clear, { f.getObject<QList>(0).data.clear(); })
BIND_F(add, listPush)
BIND_F(remove, listRemove)
BIND_F(removeIf, listRemoveIf)
BIND_F(push, listPush)
BIND_F(pop, listPop)
BIND_F(insert, listInsert)
BIND_F(removeAt, listRemoveAt)
BIND_F(indexOf, listIndexOf)
BIND_F(lastIndexOf, listLastIndexOf)
BIND_F(sort, listSort)
BIND_F(reverse, listReverse)
BIND_F(rotate, listRotate)
BIND_F(lower, listLowerBound)
BIND_F(upper, listUpperBound)
BIND_F(==, listEquals)
BIND_F(*, listTimes)
;

listIteratorClass
->copyParentMethods()
BIND_F(next, listIteratorNext)
BIND_F(previous, listIteratorPrevious)
BIND_F(remove, listIteratorRemove)
BIND_F(add, listIteratorInsert)
BIND_F(insert, listIteratorInsert)
;

listClass -> type
->copyParentMethods()
BIND_F( (), listInstantiate)
BIND_F(of, listFromSequence)
;

//println("sizeof(QList)=%d", sizeof(QList));
}
