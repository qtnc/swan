#include "SwanLib.hpp"
#include "../vm/Heap.hpp"
#include "../vm/Fiber_inlines.hpp"
using namespace std;


static void heapInstantiateFromItems (QFiber& f) {
int start=1, finish=f.getArgCount();
QV sorter =  QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (finish>=2) {
if (f.at(1).isCallable()) { sorter=f.at(1); start++; }
else if (f.at(-1).isCallable()) { sorter=f.at(-1); finish--; }
}
QHeap* heap = f.vm.construct<QHeap>(f.vm, sorter);
f.returnValue(heap);
for (int i=start, n=finish; i<n; i++) heap->push(f.at(i));
}

static void heapInstantiateFromSequences (QFiber& f) {
int start=1, finish=f.getArgCount();
QV sorter =  QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (finish>=2) {
if (f.at(1).isCallable()) { sorter=f.at(1); start++; }
else if (f.at(-1).isCallable()) { sorter=f.at(-1); finish--; }
}
QHeap* heap = f.vm.construct<QHeap>(f.vm, sorter);
f.returnValue(heap);
for (int i=start, l=finish; i<l; i++) {
f.getObject<QSequence>(i) .copyInto(f, heap->data);
}
std::make_heap(heap->data.begin(), heap->data.end(), QVBinaryPredicate(f.vm, heap->sorter));
}

static void heapPush (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) heap.push(f.at(i));
heap.incrVersion();
}

static void heapRemove (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
QVEqualler eq(f.vm);
for (int i=1, n=f.getArgCount(); i<n; i++) {
QV x = f.at(i);
auto it = find_if(heap.data.begin(), heap.data.end(), [&](const QV& y){ return eq(y, x); });
if (it==heap.data.end()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(*it);
heap.erase(it);
heap.incrVersion();
}}}

static void heapIterator (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
auto it = f.vm.construct<QHeapIterator>(f.vm, heap);
f.returnValue(it);
}

static void heapIteratorNext (QFiber& f) {
QHeapIterator& li = f.getObject<QHeapIterator>(0);
li.checkVersion();
if (li.iterator==li.heap.data.end()) f.returnValue(QV::UNDEFINED);
else f.returnValue(*li.iterator++);
}

static void heapPop (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
f.returnValue(heap.pop());
heap.incrVersion();
}

static void heapFirst (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
f.returnValue(heap.data.size()? heap.data[0] : QV::UNDEFINED);
}

static void heapLength (QFiber& f) {
auto& heap = f.getObject<QHeap>(0);
f.returnValue(static_cast<double>(heap.data.size())); 
}

static void heapClear (QFiber& f) {
auto& heap = f.getObject<QHeap>(0);
heap.data.clear();
}

static void heapToString (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
string re = "[";
heap.join(f, ", ", re);
re += "]";
f.returnValue(re);
}


void QVM::initHeapType () {
heapClass
->copyParentMethods()
->bind("iterator", heapIterator)
->bind("toString", heapToString)
->bind("length", heapLength)
->bind("clear", heapClear)
->bind("push", heapPush)
->bind("remove", heapRemove)
->bind("pop", heapPop)
->bind("first", heapFirst)
;

heapIteratorClass
->copyParentMethods()
->bind("next", heapIteratorNext)
;

heapClass ->type
->copyParentMethods()
->bind("()", heapInstantiateFromSequences)
->bind("of", heapInstantiateFromItems)
;
}

