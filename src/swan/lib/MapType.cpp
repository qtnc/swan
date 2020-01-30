#include "../../include/cpprintf.hpp"
#include "SwanLib.hpp"
#include "../vm/Map.hpp"
#include "../vm/Tuple.hpp"
using namespace std;

static void mapIn (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
auto it = map.map.find(f.at(1));
f.returnValue(it!=map.map.end());
}

static void mapInstantiateFromEntries (QFiber& f) {
QMap* map = f.vm.construct<QMap>(f.vm);
f.returnValue(map);
vector<QV, trace_allocator<QV>> tuple(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
tuple.clear();
f.getObject<QSequence>(i) .copyInto(f, tuple);
if (tuple.size()) map->map[tuple[0]] = tuple.back();
}
}

static void mapInstantiateFromMappings (QFiber& f) {
QMap* map = f.vm.construct<QMap>(f.vm);
f.returnValue(map);
vector<QV, trace_allocator<QV>> pairs(f.vm), tuple(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
pairs.clear();
f.getObject<QSequence>(i) .copyInto(f, pairs);
for (QV& pair: pairs) {
tuple.clear();
pair.asObject<QSequence>()->copyInto(f, tuple);
if (tuple.size()) map->map[tuple[0]] = tuple.back();
}}
}

static void mapIterator (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
auto it = f.vm.construct<QMapIterator>(f.vm, map);
f.returnValue(it);
}

static void mapIteratorNext (QFiber& f) {
QMapIterator& mi = f.getObject<QMapIterator>(0);
mi.checkVersion();
if (mi.iterator==mi.map.map.end()) f.returnValue(QV::UNDEFINED);
else {
QV data[] = { mi.iterator->first, mi.iterator->second };
QTuple* tuple = QTuple::create(f.vm, 2, data);
++mi.iterator;
f.returnValue(tuple);
}}

static void mapToString (QFiber& f) {
bool first = true;
string out;
out += '{';
for (auto& p: f.getObject<QMap>(0).map) {
if (!first) out +=  ", ";
QV key = p.first, value = p.second;
appendToString(f, key, out);
out+= ": ";
appendToString(f, value, out);
first=false;
}
out += '}';
f.returnValue(out);
}

static void mapRemove (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) {
auto it = map.map.find(f.at(i));
if (it==map.map.end()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(it->second);
map.map.erase(it);
map.incrVersion();
}}}

void QVM::initMapType () {
mappingClass
->copyParentMethods()
;

mapClass
->copyParentMethods()
BIND_L( [], { f.returnValue(f.getObject<QMap>(0) .get(f.at(1))); })
BIND_L( []=, { f.returnValue(f.getObject<QMap>(0) .set(f.at(1), f.at(2))); })
->bind("in", mapIn)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QMap>(0).map.size())); })
->bind("toString", mapToString)
->bind("iterator", mapIterator)
BIND_L(clear, { f.getObject<QMap>(0).map.clear(); })
->bind("remove", mapRemove)
BIND_L(reserve, { f.getObject<QMap>(0).map.reserve(f.getNum(1)); })
;

mapIteratorClass
->copyParentMethods()
->bind("next", mapIteratorNext)
;

mapClass ->type
->copyParentMethods()
->bind("()", mapInstantiateFromMappings)
->bind("of", mapInstantiateFromEntries)
;

//println("sizeof(QMap)=%d", sizeof(QMap));
}
