#ifndef NO_OPTIONAL_COLLECTIONS
#include "../../include/cpprintf.hpp"
#include "SwanLib.hpp"
#include "../vm/LinkedList.hpp"
using namespace std;

static void linkedListInstantiateFromItems (QFiber& f) {
int n = f.getArgCount() -1;
QLinkedList* list = f.vm.construct<QLinkedList>(f.vm);
f.returnValue(list);
if (n>0) list->data.insert(list->data.end(), &f.at(1), &f.at(1) +n);
}

static void linkedListIterator (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
int index = f.getOptionalNum(1, 0);
if (index<0) index += list.data.size() +1;
auto it = f.vm.construct<QLinkedListIterator>(f.vm, list);
if (index>0) std::advance(it->iterator, index);
f.returnValue(it);
}

static void linkedListIteratorNext (QFiber& f) {
QLinkedListIterator& li = f.getObject<QLinkedListIterator>(0);
li.checkVersion();
if (li.iterator==li.list.data.end()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*li.iterator++);
li.forward=true;
}

static void linkedListIteratorPrevious (QFiber& f) {
QLinkedListIterator& li = f.getObject<QLinkedListIterator>(0);
li.checkVersion();
if (li.iterator==li.list.data.begin()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*--li.iterator);
li.forward=false;
}

static void linkedListIteratorRemove (QFiber& f) {
QLinkedListIterator& li = f.getObject<QLinkedListIterator>(0);
li.checkVersion();
if (li.forward) --li.iterator;
f.returnValue(*li.iterator);
if (li.forward) li.list.data.erase(li.iterator++);
else li.list.data.erase(li.iterator--);
li.incrVersion();
}

static void linkedListIteratorInsert (QFiber& f) {
QLinkedListIterator& li = f.getObject<QLinkedListIterator>(0);
li.checkVersion();
QV value = f.at(1);
li.iterator = li.list.data.insert(li.iterator, value);
if (li.forward) ++li.iterator;
}

static void linkedListPush (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
int n = f.getArgCount() -1;
if (n>0) list.data.insert(list.data.end(), &f.at(1), (&f.at(1))+n);
f.returnValue(true);
}

static void linkedListPop (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
if (list.data.empty()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(list.data.back());
list.data.pop_back();
list.incrVersion();
}}

static void linkedListUnshift (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
int n = f.getArgCount() -1;
if (n>0) list.data.insert(list.data.begin(), &f.at(1), (&f.at(1))+n);
}

static void linkedListShift (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
if (list.data.empty()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(list.data.front());
list.data.pop_front();
list.incrVersion();
}}

static void linkedListRemove (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
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

static void linkedListRemoveIf (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
list.version++;
for (int i=1, l=f.getArgCount(); i<l; i++) {
QVUnaryPredicate pred(f.vm, f.at(i));
auto it = remove_if(list.data.begin(), list.data.end(), pred);
list.data.erase(it, list.data.end());
list.incrVersion();
}}

static void linkedListInstantiateFromSequences (QFiber& f) {
QLinkedList* list = f.vm.construct<QLinkedList>(f.vm);
f.returnValue(list);
for (int i=1, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i) .copyInto(f, *list);
}
}

static void linkedListToString (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
string re = "[^ ";
list.join(f, ", ", re);
re += "]";
f.returnValue(re);
}

void QVM::initLinkedListType () {
linkedListClass
->copyParentMethods()
->bind("push", linkedListPush)
->bind("pop", linkedListPop)
->bind("shift", linkedListShift)
->bind("unshift", linkedListUnshift)
->bind("remove", linkedListRemove)
->bind("removeIf", linkedListRemoveIf)
->bind("toString", linkedListToString)
->bind("iterator", linkedListIterator)
;

linkedListIteratorClass
->copyParentMethods()
->bind("next", linkedListIteratorNext)
->bind("previous", linkedListIteratorPrevious)
->bind("add", linkedListIteratorInsert)
->bind("insert", linkedListIteratorInsert)
->bind("remove", linkedListIteratorRemove)
;

linkedListClass ->type
->copyParentMethods()
->bind("()", linkedListInstantiateFromSequences)
->bind("of", linkedListInstantiateFromItems)
;

//println("sizeof(QLinkedList)=%d", sizeof(QLinkedList));
}
#endif
