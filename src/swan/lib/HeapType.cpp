#include "SwanLib.hpp"
#include "../vm/Heap.hpp"
#include "../vm/Fiber_inlines.hpp"
using namespace std;


static void heapInstantiate (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (!sorter.isCallable()) f.runtimeError("Sorter must be callable");
QHeap* heap = f.vm.construct<QHeap>(f.vm, sorter);
f.returnValue(heap);
for (int i=2, n=f.getArgCount(); i<n; i++) heap->push(f.at(i));
}

static void heapFromSequence (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (!sorter.isCallable()) f.runtimeError("Sorter must be callable");
QHeap* heap = f.vm.construct<QHeap>(f.vm, sorter);
f.returnValue(heap);
for (int i=2, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i) .copyInto(f, heap->data);
}
std::make_heap(heap->data.begin(), heap->data.end(), QVBinaryPredicate(f.vm, heap->sorter));
}

static void heapPush (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) heap.push(f.at(i));
}

static void heapRemove (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
QVEqualler eq(f.vm);
for (int i=1, n=f.getArgCount(); i<n; i++) {
QV x = f.at(i);
auto it = find_if(heap.data.begin(), heap.data.end(), [&](const QV& y){ return eq(y, x); });
if (it==heap.data.end()) f.returnValue(QV());
else {
f.returnValue(*it);
heap.erase(it);
}}}

static void heapIterator (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
auto it = f.vm.construct<QHeapIterator>(f.vm, heap);
f.returnValue(it);
}

static void heapIteratorHasNext (QFiber& f) {
QHeapIterator& li = f.getObject<QHeapIterator>(0);
f.returnValue(li.iterator != li.heap.data.end() );
}

static void heapIteratorNext (QFiber& f) {
QHeapIterator& li = f.getObject<QHeapIterator>(0);
f.returnValue(*li.iterator++);
}

static void heapPop (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
f.returnValue(heap.pop());
}

static void heapFirst (QFiber& f) {
QHeap& heap = f.getObject<QHeap>(0);
f.returnValue(heap.data.size()? heap.data[0] : QV());
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
BIND_F(iterator, heapIterator)
BIND_F(toString, heapToString)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QHeap>(0).data.size())); })
BIND_L(clear, { f.getObject<QHeap>(0).data.clear(); })
BIND_F(push, heapPush)
BIND_F(remove, heapRemove)
BIND_F(pop, heapPop)
BIND_F(first, heapFirst)
;

heapIteratorClass
->copyParentMethods()
BIND_F(next, heapIteratorNext)
BIND_F(hasNext, heapIteratorHasNext)
;

heapClass ->type
->copyParentMethods()
BIND_F( (), heapInstantiate)
BIND_F(of, heapFromSequence)
;
}

