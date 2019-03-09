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

static void linkedListIterate (QFiber& f) {
QLinkedList& list = f.getObject<QLinkedList>(0);
if (f.isNull(1)) {
f.returnValue(f.vm.construct<QLinkedListIterator>(f.vm, list));
}
else {
QLinkedListIterator& mi = f.getObject<QLinkedListIterator>(1);
bool cont = mi.iterator != list.data.end();
f.returnValue( cont? f.at(1) : QV());
}}

static void linkedListIteratorValue (QFiber& f) {
QLinkedListIterator& mi = f.getObject<QLinkedListIterator>(1);
QV val = *mi.iterator++;
f.returnValue(val);
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
BIND_F(iterate, linkedListIterate)
BIND_F(iteratorValue, linkedListIteratorValue)
;

linkedListMetaClass
->copyParentMethods()
BIND_F( (), linkedListInstantiate)
BIND_F(of, linkedListFromSequence)
;

//println("sizeof(QLinkedList)=%d", sizeof(QLinkedList));
}
#endif
