#include "Value.hpp"
#include "VM.hpp"
#include "Closure.hpp"
#include "Function.hpp"
#include "Upvalue.hpp"
#include "BoundFunction.hpp"
#include "List.hpp"
#include "Map.hpp"
#include "Set.hpp"
#include "Tuple.hpp"
#include "String.hpp"
#include "Range.hpp"
#include "../../include/cpprintf.hpp"
#include<algorithm>
using namespace std;

void gcDefragMem ();

bool QV::gcVisit (QVM& vm) {
if (!isObject()) return false;
return getClass(vm) .gcInfo ->gcVisit(asObject<QObject>());
}

bool QObject::gcVisit () {
if (gcMark()) return true;
type->gcVisit();
return false;
}

bool QInstance::gcVisit () {
if (QObject::gcVisit()) return true;
for (auto it = &fields[0], end = &fields[type->nFields]; it<end; ++it) it->gcVisit(type->vm);
return false;
}

bool QClass::gcVisit () {
if (QObject::gcVisit()) return true;
if (parent) parent->gcVisit();
for (int i=0, n=type->nFields; i<n; i++) staticFields[i].gcVisit(type->vm);
for (QV& val: methods) val.gcVisit(type->vm);
return false;
}

bool QFunction::gcVisit () {
if (QObject::gcVisit()) return true;
for (auto cst = constants; cst<constantsEnd; ++cst) cst->gcVisit(type->vm);
return false;
}

bool QClosure::gcVisit () {
if (QObject::gcVisit()) return true;
func.gcVisit();
for (auto upv = upvalues, end = upvalues + (func.upvaluesEnd - func.upvalues); upv<end; ++upv) (*upv)->gcVisit();
return false;
}

bool Upvalue::gcVisit () {
if (QObject::gcVisit()) return true;
if (closedValue.i==QV_OPEN_UPVALUE_MARK) fiber->gcVisit();
get().gcVisit(type->vm);
return false;
}

bool QFiber::gcVisit () {
if (QObject::gcVisit()) return true;
for (QV& val: stack) val.gcVisit(type->vm);
for (auto& cf: callFrames) if (cf.closure) cf.closure->gcVisit();
for (auto upv: openUpvalues) if (upv) upv->gcVisit();
if (parentFiber) parentFiber->gcVisit();
return false;
}

bool BoundFunction::gcVisit () {
if (QObject::gcVisit()) return true;
method.gcVisit(type->vm);
for (size_t i=0, n=count; i<n; i++) args[i].gcVisit(type->vm);
return false;
}

bool QList::gcVisit () {
if (QObject::gcVisit()) return true;
for (QV& val: data) val.gcVisit(type->vm);
return false;
}

bool QMap::gcVisit () {
if (QObject::gcVisit()) return true;
for (auto& p: map) {
const_cast<QV&>(p.first) .gcVisit(type->vm);
p.second.gcVisit(type->vm);
}
return false;
}

bool QSet::gcVisit () {
if (QObject::gcVisit()) return true;
for (const QV& val: set) const_cast<QV&>(val).gcVisit(type->vm);
return false;
}

bool QTuple::gcVisit () {
if (QObject::gcVisit()) return true;
for (uint32_t i=0; i<length; i++) data[i].gcVisit(type->vm);
return false;
}

bool QRangeIterator::gcVisit () {
if (QObject::gcVisit()) return true;
range.gcVisit();
return false;
}

bool QStringIterator::gcVisit () {
if (QObject::gcVisit()) return true;
str.gcVisit();
return false;
}

bool QListIterator::gcVisit () {
if (QObject::gcVisit()) return true;
list.gcVisit();
return false;
}

bool QTupleIterator::gcVisit () {
if (QObject::gcVisit()) return true;
tuple.gcVisit();
return false;
}

bool QMapIterator::gcVisit () {
if (QObject::gcVisit()) return true;
map.gcVisit();
return false;
}

bool QSetIterator::gcVisit () {
if (QObject::gcVisit()) return true;
set.gcVisit();
return false;
}

#ifndef NO_OPTIONAL_COLLECTIONS
#include "LinkedList.hpp"
#include "Dictionary.hpp"
#include "Heap.hpp"
#include "SortedSet.hpp"
#include "Deque.hpp"

bool QLinkedList::gcVisit () {
if (QObject::gcVisit()) return true;
for (QV& val: data) val.gcVisit(type->vm);
return false;
}

bool QDeque::gcVisit () {
if (QObject::gcVisit()) return true;
for (QV& val: data) val.gcVisit(type->vm);
return false;
}

bool QDictionary::gcVisit () {
if (QObject::gcVisit()) return true;
sorter.gcVisit(type->vm);
for (auto& p: map) {
const_cast<QV&>(p.first) .gcVisit(type->vm);
p.second.gcVisit(type->vm);
}
return false;
}

bool QSortedSet::gcVisit () {
if (QObject::gcVisit()) return true;
sorter.gcVisit(type->vm);
for (const QV& item: set) const_cast<QV&>(item).gcVisit(type->vm);
return false;
}

bool QHeap::gcVisit () {
if (QObject::gcVisit()) return true;
sorter.gcVisit(type->vm);
for (const QV& item: data) const_cast<QV&>(item).gcVisit(type->vm);
return false;
}

bool QDictionaryIterator::gcVisit () {
if (QObject::gcVisit()) return true;
map.gcVisit();
return false;
}

bool QSortedSetIterator::gcVisit () {
if (QObject::gcVisit()) return true;
set.gcVisit();
return false;
}

bool QLinkedListIterator::gcVisit () {
if (QObject::gcVisit()) return true;
list.gcVisit();
return false;
}

bool QDequeIterator::gcVisit () {
if (QObject::gcVisit()) return true;
deque.gcVisit();
return false;
}

bool QHeapIterator::gcVisit () {
if (QObject::gcVisit()) return true;
heap.gcVisit();
return false;
}
#endif

#ifndef NO_GRID
#include "Grid.hpp"

bool QGrid::gcVisit () {
if (QObject::gcVisit()) return true;
for (uint32_t i=0, length=width*height; i<length; i++) data[i].gcVisit(type->vm);
return false;
}

bool QGridIterator::gcVisit () {
if (QObject::gcVisit()) return true;
grid.gcVisit();
return false;
}
#endif

#ifndef NO_REGEX
#include "Regex.hpp"

bool QRegexIterator::gcVisit () {
if (QObject::gcVisit()) return true;
str.gcVisit();
regex.gcVisit();
return false;
}

bool QRegexTokenIterator::gcVisit () {
if (QObject::gcVisit()) return true;
str.gcVisit();
regex.gcVisit();
return false;
}
#endif

static inline void unmarkAll (QObject* initial) {
for (auto it=initial; it; it = it->gcNext()) {
it->gcUnmark();
}}

static inline void doSweep (QVM& vm, QObject*& initial) {
QObject* prev = nullptr;
size_t used=0, collected=0, total = 0;
for (QObject* ptr = initial; ptr; ) {
total++;
if (ptr->gcMarked()) {
if (!prev) initial=ptr;
prev=ptr;
ptr = ptr->gcNext();
used++;
}
else {
auto next = ptr->gcNext();
if (prev) prev->gcNext(next);
auto gci = ptr->type->gcInfo;
void* origin = gci->gcOrigin(ptr);
size_t size = gci->gcMemSize(ptr);
gci->gcDestroy(ptr);
vm.deallocate(origin, size);
ptr = next;
collected++;
}
}
if (prev) prev->gcNext(nullptr);
//println(std::cerr, "GC: %d/%d used (%d%%), %d/%d collected (%d%%)", used, total, 100*used/total, collected, total, 100*collected/total);
}

void QVM::garbageCollect () {
//println(std::cerr, "Starting GC! mem used = %d, treshhold = %d", gcMemUsage, gcTreshhold);

unmarkAll(firstGCObject);

vector<QObject*> roots = { 
boolClass, classClass, fiberClass, functionClass, iterableClass, iteratorClass, listClass, mapClass, mappingClass, nullClass, numClass, objectClass, rangeClass, setClass, stringClass, tupleClass, undefinedClass
, listIteratorClass, mapIteratorClass, rangeIteratorClass, setIteratorClass, stringIteratorClass, tupleIteratorClass
#ifndef NO_REGEX
, regexClass, regexMatchResultClass, regexIteratorClass, regexTokenIteratorClass
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
, dequeClass, dictionaryClass, linkedListClass, heapClass, sortedSetClass
, dequeIteratorClass, dictionaryIteratorClass, linkedListIteratorClass, heapIteratorClass, sortedSetIteratorClass
#endif
#ifndef NO_GRID
, gridClass, gridIteratorClass
#endif
#ifndef NO_RANDOM
, randomClass
#endif
};
roots.insert(roots.end(), fibers.begin(), fibers.end());
for (QObject* obj: roots) obj->gcVisit();
for (QV& gv: globalVariables) gv.gcVisit(*this);
for (auto& kh: keptHandles) QV(kh.first).gcVisit(*this);
for (auto& im: imports) im.second.gcVisit(*this);

auto prevMemUsage = gcMemUsage;
doSweep(*this, firstGCObject);
gcTreshhold = std::max(gcTreshhold, gcMemUsage * gcTreshholdFactor / 100);
//gcTreshhold = gcTreshholdFactor * gcTreshhold / 100;
//println(std::cerr, "GC finished: %d => %d (%d%%)", prevMemUsage, gcMemUsage, 100 * gcMemUsage / prevMemUsage);
}
