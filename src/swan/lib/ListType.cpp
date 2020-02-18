#include "../../include/cpprintf.hpp"
#include "SwanLib.hpp"
#include "../vm/List.hpp"
#include "../vm/Tuple.hpp"
using namespace std;

void checkVersion (uint32_t v1, uint32_t v2) {
if (v1!=v2) error<invalid_argument>("Concurrent modification");
}

static void listInstantiateFromItems (QFiber& f) {
int n = f.getArgCount() -1;
QList* list = f.vm.construct<QList>(f.vm);
f.returnValue(list);
if (n>0) list->data.insert(list->data.end(), &f.at(1), &f.at(1) +n);
}

static void listIterator (QFiber& f) {
QList& list = f.getObject<QList>(0);
auto it = f.vm.construct<QListIterator>(f.vm, list);
if (f.getArgCount()>=2 && f.isNum(1)) {
int index = f.getNum(1);
if (index<0) index += list.data.size() +1;
if (index>0) std::advance(it->iterator, index);
}
else if (f.getArgCount()>=2 && f.at(1).isObject() && f.at(1).isInstanceOf(f.vm.listIteratorClass) && &f.getObject<QListIterator>(1).list == &list) {
auto& it2 = f.getObject<QListIterator>(1);
it->iterator = it2.iterator;
}
f.returnValue(it);
}

static void listIteratorNext (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
li.checkVersion();
if (li.iterator==li.list.data.end()) f.returnValue(QV::UNDEFINED);
else { 
f.returnValue(*li.iterator); 
++li.iterator; 
}
li.forward=true;
}

static void listIteratorPrevious (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
li.checkVersion();
if (li.iterator==li.list.data.begin()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*--li.iterator);
li.forward=false;
}

static void listIteratorRemove (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
li.checkVersion();
if (li.forward) --li.iterator;
f.returnValue(*li.iterator);
li.iterator = li.list.data.erase(li.iterator);
li.incrVersion();
}

static void listIteratorInsert (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
li.checkVersion();
QV value = f.at(1);
li.iterator = li.list.data.insert(li.iterator, value);
if (li.forward) ++li.iterator;
li.incrVersion();
}

static void listIteratorIndex (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
li.checkVersion();
f.returnValue(static_cast<double>( li.iterator - li.list.data.begin() - (li.forward? 1 : 0) ));
}

static void listIteratorSet (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
li.checkVersion();
if (li.forward) --li.iterator;
*li.iterator = f.at(1);
if (li.forward) ++li.iterator;
f.returnValue(f.at(1));
}

static void listIteratorMinus (QFiber& f) {
QListIterator& li = f.getObject<QListIterator>(0);
li.checkVersion();
if (f.at(1).isObject() && f.at(1).isInstanceOf(f.vm.listIteratorClass) && &f.getObject<QListIterator>(1).list == &li.list) {
QListIterator& it = f.getObject<QListIterator>(1);
it.checkVersion();
f.returnValue(static_cast<double>( li.iterator - it.iterator ));
}
else f.returnValue(QV::UNDEFINED);
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
auto& dest = f.at(2);
auto& dcls = dest.getClass(f.vm);
int start, end, insertlen = dcls.gcInfo->getLength(dest.asObject<QObject>());
f.getRange(1).makeBounds(length, start, end);
list.incrVersion();
if (insertlen<0) {
list.data.erase(list.data.begin()+start, list.data.begin()+end);
auto citr = copyVisitor(std::inserter(list.data, list.data.begin()+start));
dest.copyInto(f, citr);
}
else {
int diff = insertlen - (end-start);
if (diff<0) list.data.erase(list.data.begin()+start, list.data.begin()+start-diff);
else if (diff>0) list.data.insert(list.data.begin()+start, diff, QV::UNDEFINED);
if (&dcls==f.vm.listClass) memcpy(&list.data[start], &(dest.asObject<QList>()->data[0]), sizeof(QV)*insertlen);
else if (&dcls==f.vm.tupleClass) memcpy(&list.data[start], dest.asObject<QTuple>()->data, sizeof(QV)*insertlen);
else {
auto citr = copyVisitor(list.data.begin()+start);
dest.copyInto(f, citr);
}}
f.returnValue(f.at(2));
}
else f.returnValue(QV::UNDEFINED);
}

static void listFill (QFiber& f) {
QList& list = f.getObject<QList>(0);
int n = f.getArgCount();
int from=0, to=list.data.size(); QV value = QV::UNDEFINED;
if (n==4) {
from = f.getNum(1);
to = f.getNum(2);
value = f.at(3);
Swan::Range(from, to, false).makeBounds(list.data.size(), from, to);
}
else if (n==3) {
f.getRange(1).makeBounds(list.data.size(), from, to);
value = f.at(2);
}
else if (n==2) value = f.at(1);
std::fill(list.data.begin()+from, list.data.begin()+to, value);
}

static void listResize (QFiber& f) {
QList& list = f.getObject<QList>(0);
size_t newSize = f.getNum(1);
QV value = f.getArgCount()>=3? f.at(2) : QV::UNDEFINED;
size_t curSize = list.data.size();
list.data.resize(newSize);
list.incrVersion();
if (newSize>curSize) std::fill(list.data.begin()+curSize, list.data.end(), value);
}

static void listReserve (QFiber& f) {
QList& list = f.getObject<QList>(0);
size_t capacity = f.getNum(1);
list.data.reserve(capacity);
}

static void listPush (QFiber& f) {
QList& list = f.getObject<QList>(0);
int n = f.getArgCount() -1;
if (n>0) list.data.insert(list.data.end(), &f.at(1), (&f.at(1))+n);
list.incrVersion();
f.returnValue(true);
}

static void listPop (QFiber& f) {
QList& list = f.getObject<QList>(0);
if (list.data.empty()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(list.data.back());
list.data.pop_back();
list.incrVersion();
}}

static void listInsert (QFiber& f) {
QList& list = f.getObject<QList>(0);
int n = f.getNum(1), count = f.getArgCount() -2;
auto pos = n>=0? list.data.begin()+n : list.data.end()+n;
list.data.insert(pos, &f.at(2), (&f.at(2))+count);
list.incrVersion();
}

static void listRemoveAt (QFiber& f) {
QList& list = f.getObject<QList>(0);
for (int i=1, l=f.getArgCount(); i<l; i++) {
if (f.isNum(i)) {
int n = f.getNum(i);
auto pos = n>=0? list.data.begin()+n : list.data.end()+n;
list.data.erase(pos);
list.incrVersion();
}
else if (f.isRange(i)) {
int start, end;
f.getRange(i).makeBounds(list.data.size(), start, end);
list.data.erase(list.data.begin()+start, list.data.begin()+end);
list.incrVersion();
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
list.incrVersion();
}
else f.returnValue(QV::UNDEFINED);
}}

static void listRemoveIf (QFiber& f) {
QList& list = f.getObject<QList>(0);
for (int i=1, l=f.getArgCount(); i<l; i++) {
QVUnaryPredicate pred(f.vm, f.at(i));
auto it = remove_if(list.data.begin(), list.data.end(), pred);
list.data.erase(it, list.data.end());
list.incrVersion();
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
if (f.getArgCount()>=2 && !f.at(1).isUndefined()) stable_sort(list.data.begin(), list.data.end(), QVBinaryPredicate(f.vm, f.at(1)));
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

static void listLength (QFiber& f) {
QList& list = f.getObject<QList>(0);
f.returnValue(static_cast<double>(list.data.size()));
}

static void listClear (QFiber& f) {
QList& list = f.getObject<QList>(0);
list.data.clear();
}

static void listInstantiateFromSequences (QFiber& f) {
QList* list = f.vm.construct<QList>(f.vm);
f.returnValue(list);
int totalLen = 0;
for (int i=1, l=f.getArgCount(); i<l; i++) {
int ll = f.at(i).getClass(f.vm).gcInfo->getLength(f.at(i).asObject<QObject>());
if (ll<0) { totalLen=-1; break; }
totalLen+=ll;
}
if (totalLen<0) {
auto citr = copyVisitor(std::back_inserter(list->data));
for (int i=1, l=f.getArgCount(); i<l; i++) f.at(i).copyInto(f, citr);
}
else {
list->data.resize(totalLen);
auto citr = copyVisitor(list->data.begin());
for (int i=1, l=f.getArgCount(); i<l; i++) f.at(i).copyInto(f, citr);
}}

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
->bind("[]", listSubscript)
->bind("[]=", listSubscriptSetter)
->bind("iterator", listIterator)
->bind("length", listLength)
->bind("toString", listToString)
->bind("clear", listClear)
->bind("add", listPush)
->bind("append", listPush)
->bind("remove", listRemove)
->bind("removeIf", listRemoveIf)
->bind("push", listPush)
->bind("pop", listPop)
->bind("insert", listInsert)
->bind("removeAt", listRemoveAt)
->bind("indexOf", listIndexOf)
->bind("lastIndexOf", listLastIndexOf)
->bind("sort", listSort)
->bind("fill", listFill)
->bind("reverse", listReverse)
->bind("rotate", listRotate)
->bind("lower", listLowerBound)
->bind("upper", listUpperBound)
->bind("resize", listResize)
->bind("reserve", listReserve)
->bind("==", listEquals)
->bind("*", listTimes)
->assoc<QList>();

listIteratorClass
->copyParentMethods()
->bind("next", listIteratorNext)
->bind("previous", listIteratorPrevious)
->bind("remove", listIteratorRemove)
->bind("add", listIteratorInsert)
->bind("insert", listIteratorInsert)
->bind("set", listIteratorSet)
->bind("-", listIteratorMinus)
->bind("unp", listIteratorIndex)
->assoc<QListIterator>();

listClass -> type
->copyParentMethods()
->bind("()", listInstantiateFromSequences)
->bind("of", listInstantiateFromItems)
->assoc<QClass>();

//println("sizeof(QList)=%d", sizeof(QList));
}
