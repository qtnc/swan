#include "SwanLib.hpp"
#include "../vm/PriorityQueue.hpp"
using namespace std;


static void pqInstantiate (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
QPriorityQueue* pq = new QPriorityQueue(f.vm, sorter);
for (int i=2, n=f.getArgCount(); i<n; i++) pq->push(f.at(i));
f.returnValue(pq);
}

static void pqFromSequence (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
QPriorityQueue* pq = new QPriorityQueue(f.vm, sorter);
for (int i=2, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i).insertIntoVector(f, pq->data, 0);
}
std::make_heap(pq->data.begin(), pq->data.end(), QVBinaryPredicate(pq->sorter));
f.returnValue(pq);
}

static void pqPush (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) pq.push(f.at(i));
}

static void pqRemove (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
QVEqualler eq;
for (int i=1, n=f.getArgCount(); i<n; i++) {
QV x = f.at(i);
auto it = find_if(pq.data.begin(), pq.data.end(), [&](const QV& y){ return eq(y, x); });
if (it==pq.data.end()) f.returnValue(QV());
else {
f.returnValue(*it);
pq.erase(it);
}}}

static void pqIterate (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
int index = 1 + f.getOptionalNum(1, -1);
f.returnValue(index>=pq.data.size()? QV() : static_cast<double>(index));
}

static void pqIteratorValue (QFiber& f) {
QPriorityQueue& pq = f.getObject<QPriorityQueue>(0);
int i = f.getNum(1);
f.returnValue(pq.data[i]);
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
BIND_F(iteratorValue, pqIteratorValue)
BIND_F(iterate, pqIterate)
BIND_F(toString, pqToString)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QPriorityQueue>(0).data.size())); })
BIND_L(clear, { f.getObject<QPriorityQueue>(0).data.clear(); })
BIND_F(push, pqPush)
BIND_F(remove, pqRemove)
BIND_F(pop, pqPop)
BIND_F(first, pqFirst)
;


priorityQueueMetaClass
->copyParentMethods()
BIND_F( (), pqInstantiate)
BIND_F(of, pqFromSequence)
;
}

