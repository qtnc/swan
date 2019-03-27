#include "../../include/cpprintf.hpp"
#ifndef NO_OPTIONAL_COLLECTIONS
#include "SwanLib.hpp"
#include "../vm/LinkedList.hpp"
using namespace std;

static void linkedListInstantiate (QFiber& f) {
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

static void linkedListIteratorHasNext (QFiber& f) {
QLinkedListIterator& li = f.getObject<QLinkedListIterator>(0);
f.returnValue(li.iterator != li.list.data.end() );
}

static void linkedListIteratorHasPrevious  (QFiber& f) {
QLinkedListIterator& li = f.getObject<QLinkedListIterator>(0);
f.returnValue(li.iterator != li.list.data.begin() );
}

static void linkedListIteratorNext (QFiber& f) {
QLinkedListIterator& li = f.getObject<QLinkedListIterator>(0);
f.returnValue(*li.iterator++);
}

static void linkedListIteratorPrevious (QFiber& f) {
QLinkedListIterator& li = f.getObject<QLinkedListIterator>(0);
f.returnValue(*--li.iterator);
}

static void linkedListPush (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
int n = f.getArgCount() -1;
if (n>0) list.data.insert(list.data.end(), &f.at(1), (&f.at(1))+n);
}

static void linkedListPop (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
if (list.data.empty()) f.returnValue(QV());
else {
f.returnValue(list.data.back());
list.data.pop_back();
}}

static void linkedListUnshift (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
int n = f.getArgCount() -1;
if (n>0) list.data.insert(list.data.begin(), &f.at(1), (&f.at(1))+n);
}

static void linkedListShift (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
if (list.data.empty()) f.returnValue(QV());
else {
f.returnValue(list.data.front());
list.data.pop_front();
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
}
else f.returnValue(QV());
}}

static void linkedListRemoveIf (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
for (int i=1, l=f.getArgCount(); i<l; i++) {
QVUnaryPredicate pred(f.vm, f.at(i));
auto it = remove_if(list.data.begin(), list.data.end(), pred);
list.data.erase(it, list.data.end());
}}

static void linkedListFromSequence (QFiber& f) {
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
BIND_F(push, linkedListPush)
BIND_F(pop, linkedListPop)
BIND_F(shift, linkedListShift)
BIND_F(unshift, linkedListUnshift)
BIND_F(remove, linkedListRemove)
BIND_F(removeIf, linkedListRemoveIf)
BIND_F(toString, linkedListToString)
BIND_F(iterator, linkedListIterator)
;

linkedListIteratorClass
->copyParentMethods()
BIND_F(next, linkedListIteratorNext)
BIND_F(hasNext, linkedListIteratorHasNext)
BIND_F(previous, linkedListIteratorPrevious)
BIND_F(hasPrevious, linkedListIteratorHasPrevious)
;

linkedListClass ->type
->copyParentMethods()
BIND_F( (), linkedListInstantiate)
BIND_F(of, linkedListFromSequence)
;

//println("sizeof(QLinkedList)=%d", sizeof(QLinkedList));
}
#endif
