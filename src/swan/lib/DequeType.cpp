#ifndef NO_OPTIONAL_COLLECTIONS
#include "../../include/cpprintf.hpp"
#include "SwanLib.hpp"
#include "../vm/Deque.hpp"
using namespace std;

static void dequeInstantiateFromItems (QFiber& f) {
int n = f.getArgCount() -1;
QDeque* deque = f.vm.construct<QDeque>(f.vm);
f.returnValue(deque);
if (n>0) deque->data.insert(deque->data.end(), &f.at(1), &f.at(1) +n);
}

static void dequeIterator (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
int index = f.getOptionalNum(1, 0);
if (index<0) index += deque.data.size() +1;
auto it = f.vm.construct<QDequeIterator>(f.vm, deque);
if (index>0) std::advance(it->iterator, index);
f.returnValue(it);
}

static void dequeIteratorNext (QFiber& f) {
QDequeIterator& li = f.getObject<QDequeIterator>(0);
li.checkVersion();
if (li.iterator==li.deque.data.end()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*li.iterator++);
li.forward=true;
}

static void dequeIteratorPrevious (QFiber& f) {
QDequeIterator& li = f.getObject<QDequeIterator>(0);
li.checkVersion();
if (li.iterator==li.deque.data.begin()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*--li.iterator);
li.forward=false;
}

static void dequeIteratorRemove (QFiber& f) {
QDequeIterator& li = f.getObject<QDequeIterator>(0);
li.checkVersion();
if (li.forward) --li.iterator;
f.returnValue(*li.iterator);
li.iterator = li.deque.data.erase(li.iterator);
li.incrVersion();
}

static void dequeIteratorInsert (QFiber& f) {
QDequeIterator& li = f.getObject<QDequeIterator>(0);
li.checkVersion();
QV value = f.at(1);
li.iterator = li.deque.data.insert(li.iterator, value);
if (li.forward) ++li.iterator;
li.incrVersion();
}

static void dequeIteratorSet (QFiber& f) {
QDequeIterator& li = f.getObject<QDequeIterator>(0);
li.checkVersion();
if (li.forward) --li.iterator;
*li.iterator = f.at(1);
if (li.forward) ++li.iterator;
f.returnValue(f.at(1));
}

static void dequeIteratorIndex (QFiber& f) {
QDequeIterator& li = f.getObject<QDequeIterator>(0);
li.checkVersion();
f.returnValue(static_cast<double>( li.iterator - li.deque.data.begin() - (li.forward? 1 : 0) ));
}

static void dequeIteratorMinus (QFiber& f) {
QDequeIterator& li = f.getObject<QDequeIterator>(0);
li.checkVersion();
if (f.at(1).isObject() && f.at(1).isInstanceOf(f.vm.dequeIteratorClass) && &f.getObject<QDequeIterator>(1).deque == &li.deque) {
QDequeIterator& it = f.getObject<QDequeIterator>(1);
it.checkVersion();
f.returnValue(static_cast<double>( li.iterator - it.iterator ));
}
else f.returnValue(QV::UNDEFINED);
}

static void dequeSubscript (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
int length = deque.data.size();
if (f.isNum(1)) {
int i = f.getNum(1);
if (i<0) i+=length;
f.returnValue(i>=0 && i<length? deque.data.at(i) : QV::UNDEFINED);
}
else if (f.isRange(1)) {
int start, end;
f.getRange(1).makeBounds(length, start, end);
QDeque* newDeque = f.vm.construct<QDeque>(f.vm);
f.returnValue(newDeque);
if (end-start>0) newDeque->data.insert(newDeque->data.end(), deque.data.begin()+start, deque.data.begin()+end);
}
else f.returnValue(QV::UNDEFINED);
}

static void dequeSubscriptSetter (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
int length = deque.data.size();
if (f.isNum(1)) {
int i = f.getNum(1);
if (i<0) i+=length;
f.returnValue(deque.data.at(i) = f.at(2));
}
else if (f.isRange(1)) {
int start, end;
f.getRange(1).makeBounds(length, start, end);
deque.data.erase(deque.data.begin()+start, deque.data.begin()+end);
auto citr = copyVisitor(std::inserter(deque.data, deque.data.begin()+start));
f.at(2).copyInto(f, citr);
f.returnValue(f.at(2));
deque.incrVersion();
}
else f.returnValue(QV::UNDEFINED);
}

static void dequeFill (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
int n = f.getArgCount();
int from=0, to=deque.data.size(); QV value = QV::UNDEFINED;
if (n==4) {
from = f.getNum(1);
to = f.getNum(2);
value = f.at(3);
Swan::Range(from, to, false).makeBounds(deque.data.size(), from, to);
}
else if (n==3) {
f.getRange(1).makeBounds(deque.data.size(), from, to);
value = f.at(2);
}
else if (n==2) value = f.at(1);
std::fill(deque.data.begin()+from, deque.data.begin()+to, value);
}

static void dequeResize (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
size_t newSize = f.getNum(1);
QV value = f.getArgCount()>=3? f.at(2) : QV::UNDEFINED;
size_t curSize = deque.data.size();
deque.data.resize(newSize);
if (newSize>curSize) std::fill(deque.data.begin()+curSize, deque.data.end(), value);
deque.incrVersion();
}

static void dequePush (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
int n = f.getArgCount() -1;
if (n>0) deque.data.insert(deque.data.end(), &f.at(1), (&f.at(1))+n);
deque.incrVersion();
}

static void dequePop (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
if (deque.data.empty()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(deque.data.back());
deque.data.pop_back();
deque.incrVersion();
}}

static void dequeUnshift (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
int n = f.getArgCount() -1;
if (n>0) deque.data.insert(deque.data.begin(), &f.at(1), (&f.at(1))+n);
deque.incrVersion();
}

static void dequeShift (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
if (deque.data.empty()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(deque.data.front());
deque.data.pop_front();
deque.incrVersion();
}}

static void dequeInsert (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
int n = f.getNum(1), count = f.getArgCount() -2;
auto pos = n>=0? deque.data.begin()+n : deque.data.end()+n;
deque.data.insert(pos, &f.at(2), (&f.at(2))+count);
deque.incrVersion();
}

static void dequeRemoveAt (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
for (int i=1, l=f.getArgCount(); i<l; i++) {
if (f.isNum(i)) {
int n = f.getNum(i);
auto pos = n>=0? deque.data.begin()+n : deque.data.end()+n;
deque.data.erase(pos);
deque.incrVersion();
}
else if (f.isRange(i)) {
int start, end;
f.getRange(i).makeBounds(deque.data.size(), start, end);
deque.data.erase(deque.data.begin()+start, deque.data.begin()+end);
deque.incrVersion();
}
}}

static void dequeRemove (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
QVEqualler eq(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
QV& toRemove = f.at(i);
auto it = find_if(deque.data.begin(), deque.data.end(), [&](const QV& v){ return eq(v, toRemove); });
if (it!=deque.data.end()) {
f.returnValue(*it);
deque.data.erase(it);
deque.incrVersion();
}
else f.returnValue(QV::UNDEFINED);
}}

static void dequeRemoveIf (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
for (int i=1, l=f.getArgCount(); i<l; i++) {
QVUnaryPredicate pred(f.vm, f.at(i));
auto it = remove_if(deque.data.begin(), deque.data.end(), pred);
deque.data.erase(it, deque.data.end());
deque.incrVersion();
}}

static void dequeIndexOf (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
QV& needle = f.at(1);
int start = f.getOptionalNum(2, 0);
QVEqualler eq(f.vm);
auto end = deque.data.end(), begin = start>=0? deque.data.begin()+start : deque.data.end()+start,
re = find_if(begin, end, [&](const QV& v){ return eq(v, needle); });
f.returnValue(re==end? -1.0 : static_cast<double>(re-deque.data.begin()));
}

static void dequeLastIndexOf (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
QV& needle = f.at(1);
int start = f.getOptionalNum(2, deque.data.size());
auto begin = deque.data.begin(), end = start>=0? deque.data.begin()+start : deque.data.end()+start,
re = find_end(begin, end, &needle, (&needle)+1, QVEqualler(f.vm));
f.returnValue(re==end? -1.0 : static_cast<double>(re-deque.data.begin()));
}

static void dequeLowerBound (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
QV value = f.at(1);
auto it = deque.data.end();
if (f.getArgCount()>2) it = lower_bound(deque.data.begin(), deque.data.end(), value, QVBinaryPredicate(f.vm, f.at(2)));
else it = lower_bound(deque.data.begin(), deque.data.end(), value, QVLess(f.vm));
f.returnValue(static_cast<double>(it - deque.data.begin() ));
}

static void dequeUpperBound (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
QV value = f.at(1);
auto it = deque.data.end();
if (f.getArgCount()>2) it = upper_bound(deque.data.begin(), deque.data.end(), value, QVBinaryPredicate(f.vm, f.at(2)));
else it = upper_bound(deque.data.begin(), deque.data.end(), value, QVLess(f.vm));
f.returnValue(static_cast<double>(it - deque.data.begin() ));
}

static void dequeEquals (QFiber& f) {
QDeque &l1 = f.getObject<QDeque>(0), &l2 = f.getObject<QDeque>(1);
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

static void dequeLength (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
f.returnValue(static_cast<double>(deque.data.size())); 
}

static void dequeClear (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
deque.data.clear();
}


static void dequeInstantiateFromSequences (QFiber& f) {
QDeque* deque = f.vm.construct<QDeque>(f.vm);
f.returnValue(deque);
auto citr = copyVisitor(std::back_inserter(deque->data));
for (int i=1, l=f.getArgCount(); i<l; i++) {
f.at(i).copyInto(f, citr);
}
}

static void dequeToString (QFiber& f) {
QDeque& deque = f.getObject<QDeque>(0);
string re = "[";
deque.join(f, ", ", re);
re += "]";
f.returnValue(re);
}


void QVM::initDequeType () {
dequeClass
->copyParentMethods()
->bind("[]", dequeSubscript)
->bind("[]=", dequeSubscriptSetter)
->bind("iterator", dequeIterator)
->bind("length", dequeLength)
->bind("toString", dequeToString)
->bind("clear", dequeClear)
->bind("add", dequePush)
->bind("remove", dequeRemove)
->bind("removeIf", dequeRemoveIf)
->bind("push", dequePush)
->bind("pop", dequePop)
->bind("shift", dequeShift)
->bind("unshift", dequeUnshift)
->bind("insert", dequeInsert)
->bind("removeAt", dequeRemoveAt)
->bind("indexOf", dequeIndexOf)
->bind("lastIndexOf", dequeLastIndexOf)
->bind("lower", dequeLowerBound)
->bind("upper", dequeUpperBound)
->bind("fill", dequeFill)
->bind("resize", dequeResize)
->bind("==", dequeEquals)
->assoc<QDeque>();

dequeIteratorClass
->copyParentMethods()
->bind("next", dequeIteratorNext)
->bind("previous", dequeIteratorPrevious)
->bind("remove", dequeIteratorRemove)
->bind("add", dequeIteratorInsert)
->bind("insert", dequeIteratorInsert)
->bind("set", dequeIteratorSet)
->bind("unp", dequeIteratorIndex)
->bind("-", dequeIteratorMinus)
->assoc<QDequeIterator>();

dequeClass -> type
->copyParentMethods()
->bind("()", dequeInstantiateFromSequences)
->bind("of", dequeInstantiateFromItems)
->assoc<QClass>();

//println("sizeof(QDeque)=%d", sizeof(QDeque));
}

#endif
