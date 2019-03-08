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

static void mapInstantiate (QFiber& f) {
QMap* map = f.vm.construct<QMap>(f.vm);
f.returnValue(map);
vector<QV, trace_allocator<QV>> tuple(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
tuple.clear();
f.getObject<QSequence>(i) .copyInto(f, tuple);
map->map[tuple[0]] = tuple.back();
}
}

static void mapFromSequence (QFiber& f) {
QMap* map = f.vm.construct<QMap>(f.vm);
f.returnValue(map);
vector<QV, trace_allocator<QV>> pairs(f.vm), tuple(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
pairs.clear();
f.getObject<QSequence>(i) .copyInto(f, pairs);
for (QV& pair: pairs) {
tuple.clear();
pair.asObject<QSequence>()->copyInto(f, tuple);
map->map[tuple[0]] = tuple.back();
}}
}

static void mapIterate (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
if (f.isNull(1)) {
f.returnValue(f.vm.construct<QMapIterator>(f.vm, map));
}
else {
QMapIterator& mi = f.getObject<QMapIterator>(1);
bool cont = mi.iterator != map.map.end();
f.returnValue( cont? f.at(1) : QV());
}}

static void mapIteratorValue (QFiber& f) {
QMapIterator& mi = f.getObject<QMapIterator>(1);
QV data[] = { mi.iterator->first, mi.iterator->second };
QTuple* tuple = QTuple::create(f.vm, 2, data);
++mi.iterator;
f.returnValue(tuple);
}

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
if (it==map.map.end()) f.returnValue(QV());
else {
f.returnValue(it->second);
map.map.erase(it);
}}}


void QVM::initMapType () {
mapClass
->copyParentMethods()
BIND_L( [], { f.returnValue(f.getObject<QMap>(0) .get(f.at(1))); })
BIND_L( []=, { f.returnValue(f.getObject<QMap>(0) .set(f.at(1), f.at(2))); })
BIND_F(in, mapIn)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QMap>(0).map.size())); })
BIND_F(toString, mapToString)
BIND_F(iterate, mapIterate)
BIND_F(iteratorValue, mapIteratorValue)
BIND_L(clear, { f.getObject<QMap>(0).map.clear(); })
BIND_F(remove, mapRemove)
;

mapMetaClass
->copyParentMethods()
BIND_F( (), mapInstantiate)
BIND_F(of, mapFromSequence)
;

println("sizeof(QMap)=%d", sizeof(QMap));
}
