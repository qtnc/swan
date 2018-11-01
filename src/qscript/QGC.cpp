#include "QValue.hpp"
#include "QValueExt.hpp"
#include "../include/cpprintf.hpp"
#include<algorithm>
using namespace std;

struct GCOIterator {
QObject* cur;
bool first;
GCOIterator (QObject* initial): cur(initial), first(true)  {}
inline QObject& operator* () { return *cur; }
bool hasNext () {
auto old = cur;
if (first) first=false;
else cur = reinterpret_cast<QObject*>( reinterpret_cast<uintptr_t>(cur->next) &~1 );
if (cur && reinterpret_cast<uintptr_t>(cur) <= 0x1000) {
print("Warning: cur=%#p, old=%#p: ", cur, old);
println("%s", QV(old).print());
}
return !!cur;
}};

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

bool QObject::gcVisit () {
if (mark(*this)) return true;
type->gcVisit();
return false;
}

bool QInstance::gcVisit () {
if (QObject::gcVisit()) return true;
for (QV& value: fields) value.gcVisit();
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
for (int i=0, n=func.upvalues.size(); i<n; i++) upvalues[i]->value.gcVisit();
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
for (auto& cf: callFrames) cf.closure->gcVisit();
for (auto upv: openUpvalues) upv->gcVisit();
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

QVM::~QVM () {
GCOIterator it(firstGCObject);
vector<QObject*> toDelete;
while(it.hasNext())toDelete.push_back(&*it);
for (auto& obj: toDelete) delete obj;
}

void QVM::garbageCollect () {
//println("Starting GC !");

int count = 0;
GCOIterator it(firstGCObject);
while(it.hasNext()){
count++;
unmark(*it);
}
//println("%d allocated objects found", count);

vector<QObject*> roots = { 
boolClass, bufferClass, classClass, fiberClass, functionClass, listClass, mapClass, nullClass, numClass, objectClass, rangeClass, setClass, sequenceClass, stringClass, tupleClass,
regexClass, regexMatchResultClass, regexIteratorClass, regexTokenIteratorClass,
QFiber::curFiber
};
for (QObject* obj: roots) obj->gcVisit();
for (QV& gv: globalVariables) gv.gcVisit();

vector<QObject*> toDelete;
int used=0, collectable=0;
count = 0;
it = GCOIterator(firstGCObject);
QObject* prev = nullptr;
while(it.hasNext()){
bool m = marked(*it);
if (m) used++;
else collectable++;
count++;
QV val(&*it);
//println("%d. %s, %s", count, val.print(), marked(*it)?"used":"collectable");
if (!m) {
if (prev) prev->next = (*it).next;
toDelete.push_back(&*it);
}
else { 
if (!prev) firstGCObject = &*it;
prev = &*it;
}
}
for (auto obj: toDelete) delete obj;
//println("%d used objects, %d collectable objects", used, collectable);
}
