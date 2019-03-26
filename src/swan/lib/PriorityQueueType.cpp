#include "SwanLib.hpp"
#include "../vm/PriorityQueue.hpp"
#include "../vm/Fiber_inlines.hpp"
using namespace std;


static void pqInstantiate (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (!sorter.isCallable()) f.runtimeError("Sorter must be callable");
QPriorityQueue* pq = f.vm.construct<QPriorityQueue>(f.vm, sorter);
f.returnValue(pq);
for (int i=2, n=f.getArgCount(); i<n; i++) pq->push(f.at(i));
}

static void pqFromSequence (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (!sorter.isCallable()) f.runtimeError("Sorter must be callable");
QPriorityQueue* pq = f.vm.construct<QPriorityQueue>(f.vm, sorter);
f.returnValue(pq);
for (int i=2, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i) .copyInto(f, pq->data);
}
std::make_heap(pq->data.begin(), pq->data.end(), QVBinaryPredicate(f.vm, pq->sorter));
}

static void pqPush (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) pq.push(f.at(i));
}

static void pqRemove (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
QVEqualler eq(f.vm);
for (int i=1, n=f.getArgCount(); i<n; i++) {
QV x = f.at(i);
auto it = find_if(pq.data.begin(), pq.data.end(), [&](const QV& y){ return eq(y, x); });
if (it==pq.data.end()) f.returnValue(QV());
else {
f.returnValue(*it);
pq.erase(it);
}}}

static void pqIterator (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
auto it = f.vm.construct<QPriorityQueueIterator>(f.vm, pq);
f.returnValue(it);
}

static void pqIteratorHasNext (QFiber& f) {
QPriorityQueueIterator& li = f.getObject<QPriorityQueueIterator>(0);
f.returnValue(li.iterator != li.pq.data.end() );
}

static void pqIteratorNext (QFiber& f) {
QPriorityQueueIterator& li = f.getObject<QPriorityQueueIterator>(0);
f.returnValue(*li.iterator++);
}

static void pqPop (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
f.returnValue(pq.pop());
}

static void pqFirst (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
f.returnValue(pq.data.size()? pq.data[0] : QV());
}

static void pqToString (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
string re = "[";
pq.join(f, ", ", re);
re += "]";
f.returnValue(re);
}


void QVM::initPriorityQueueType () {
priorityQueueClass
->copyParentMethods()
BIND_F(iterator, pqIterator)
BIND_F(toString, pqToString)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QPriorityQueue>(0).data.size())); })
BIND_L(clear, { f.getObject<QPriorityQueue>(0).data.clear(); })
BIND_F(push, pqPush)
BIND_F(remove, pqRemove)
BIND_F(pop, pqPop)
BIND_F(first, pqFirst)
;

priorityQueueIteratorClass
->copyParentMethods()
BIND_F(next, pqIteratorNext)
BIND_F(hasNext, pqIteratorHasNext)
;

priorityQueueClass ->type
->copyParentMethods()
BIND_F( (), pqInstantiate)
BIND_F(of, pqFromSequence)
;
}

