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

static inline bool marked (QObject& obj) {
return reinterpret_cast<uintptr_t>(obj.next) &1;
}

static inline bool mark (QObject& obj) {
if (marked(obj)) return true;
obj.next = reinterpret_cast<QObject*>( reinterpret_cast<uintptr_t>(obj.next) | 1);
return false;
}

static inline void unmark (QObject& obj) {
obj.next = reinterpret_cast<QObject*>( reinterpret_cast<uintptr_t>(obj.next) &~1);
}

static inline QObject* to_ptr (QObject* p) {
return reinterpret_cast<QObject*>(reinterpret_cast<uintptr_t>(p) &~3);
}

bool QObject::gcVisit () {
if (mark(*this)) return true;
type->gcVisit();
return false;
}

bool QInstance::gcVisit () {
if (QObject::gcVisit()) return true;
for (auto it = &fields[0], end = &fields[type->nFields]; it<end; ++it) it->gcVisit();
return false;
}

bool QClass::gcVisit () {
if (QObject::gcVisit()) return true;
if (parent) {
parent->gcVisit();
for (int i=0, n=parent->nFields; i<n; i++) staticFields[i].gcVisit();
}
for (QV& val: methods) val.gcVisit();
return false;
}

bool QFunction::gcVisit () {
if (QObject::gcVisit()) return true;
for (auto& cst: constants) cst.gcVisit();
return false;
}

bool QClosure::gcVisit () {
if (QObject::gcVisit()) return true;
func.gcVisit();
for (int i=0, n=func.upvalues.size(); i<n; i++) upvalues[i]->gcVisit();
return false;
}

bool Upvalue::gcVisit () {
if (QObject::gcVisit()) return true;
if (value.isOpenUpvalue()) fiber->gcVisit();
get().gcVisit();
return false;
}

bool QFiber::gcVisit () {
if (QObject::gcVisit()) return true;
for (QV& val: stack) val.gcVisit();
for (auto& cf: callFrames) if (cf.closure) cf.closure->gcVisit();
for (auto upv: openUpvalues) if (upv) upv->gcVisit();
if (parentFiber) parentFiber->gcVisit();
return false;
}

bool QList::gcVisit () {
if (QObject::gcVisit()) return true;
for (QV& val: data) val.gcVisit();
return false;
}

bool QMap::gcVisit () {
if (QObject::gcVisit()) return true;
for (auto& p: map) {
const_cast<QV&>(p.first) .gcVisit();
p.second.gcVisit();
}
return false;
}

bool QSet::gcVisit () {
if (QObject::gcVisit()) return true;
for (const QV& val: set) const_cast<QV&>(val).gcVisit();
return false;
}

bool QTuple::gcVisit () {
if (QObject::gcVisit()) return true;
for (uint32_t i=0; i<length; i++) data[i].gcVisit();
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

bool QLinkedList::gcVisit () {
if (QObject::gcVisit()) return true;
for (QV& val: data) val.gcVisit();
return false;
}

bool QDictionary::gcVisit () {
if (QObject::gcVisit()) return true;
sorter.gcVisit();
for (auto& p: map) {
const_cast<QV&>(p.first) .gcVisit();
p.second.gcVisit();
}
return false;
}

bool QSortedSet::gcVisit () {
if (QObject::gcVisit()) return true;
sorter.gcVisit();
for (const QV& item: set) const_cast<QV&>(item).gcVisit();
return false;
}

bool QHeap::gcVisit () {
if (QObject::gcVisit()) return true;
sorter.gcVisit();
for (const QV& item: data) const_cast<QV&>(item).gcVisit();
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
for (uint32_t i=0, length=width*height; i<length; i++) data[i].gcVisit();
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

#ifndef NO_BUFFER
#include "Buffer.hpp"
bool QBufferIterator::gcVisit () {
if (QObject::gcVisit()) return true;
buf.gcVisit();
return false;
}
#endif


static QV makeqv (QObject* obj) {
#define T(C,G) if (dynamic_cast<C*>(obj)) return QV(obj, G);
T(QClosure, QV_TAG_CLOSURE)
T(QFiber, QV_TAG_FIBER)
T(BoundFunction, QV_TAG_BOUND_FUNCTION)
T(QFunction, QV_TAG_NORMAL_FUNCTION)
T(Upvalue, QV_TAG_OPEN_UPVALUE)
T(QString, QV_TAG_STRING)
#undef T
return obj;
}

void QVM::garbageCollect () {
LOCK_SCOPE(gil)
//println("Starting GC! mem used = %d, treshhold = %d", gcMemUsage, gcTreshhold);

auto initial = to_ptr(firstGCObject);
for (auto it=initial; it; it = to_ptr(it->next)) unmark(*it);

vector<QObject*> roots = { 
boolClass, classClass, fiberClass, functionClass, iterableClass, iteratorClass, listClass, mapClass, nullClass, numClass, objectClass, rangeClass, setClass, stringClass, systemClass, tupleClass
, listIteratorClass, mapIteratorClass, rangeIteratorClass, setIteratorClass, stringIteratorClass, tupleIteratorClass
#ifndef NO_REGEX
, regexClass, regexMatchResultClass, regexIteratorClass, regexTokenIteratorClass
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
, dictionaryClass, linkedListClass, heapClass, sortedSetClass
, dictionaryIteratorClass, linkedListIteratorClass, heapIteratorClass, sortedSetIteratorClass
#endif
#ifndef NO_GRID
, gridClass, gridIteratorClass
#endif
#ifndef NO_RANDOM
, randomClass
#endif
#ifndef NO_BUFFER
, bufferClass, bufferIteratorClass
#endif
, activeFiber, rootFiber
};
for (QObject* obj: roots) obj->gcVisit();
for (QV& gv: globalVariables) gv.gcVisit();
for (QV& kh: keptHandles) kh.gcVisit();
for (auto& im: imports) im.second.gcVisit();

QObject* prev = nullptr;
size_t used=0, collectable=0, count = 0;
for (QObject *it=initial, *ptr=to_ptr(it); ptr; ptr=to_ptr(it)) {
//QV val = makeqv(&*ptr);
//println(std::cerr, "%d. %s, %s", count, val.print(), marked(*ptr)?"used":"collectable");
count++;
if (marked(*ptr)) {
if (!prev) initial=ptr;
prev=ptr;
it = ptr->next;
used++;
}
else {
if (prev) prev->next = ptr->next;
it = ptr->next;
size_t size = ptr->getMemSize();
ptr->~QObject();
deallocate(ptr, size);
collectable++;
}
}
//println(std::cerr, "GC Stats: %d objects ammong which %d used (%d%%) and %d collectable (%d%%)", count, used, 100*used/count, collectable, 100*collectable/count);
prev->next = nullptr;
firstGCObject = initial;
gcTreshhold = std::max(gcTreshhold, gcMemUsage * gcTreshholdFactor / 100);
//println("GC finished. mem usage = %d, new treshhold = %d", gcMemUsage, gcTreshhold);
}
