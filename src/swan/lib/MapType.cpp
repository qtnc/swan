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
auto citr = copyVisitor(std::back_inserter(tuple));
for (int i=1, l=f.getArgCount(); i<l; i++) {
tuple.clear();
f.at(i).copyInto(f, citr);
if (tuple.size()) map->map[tuple[0]] = tuple.back();
}
}

static void mapInstantiateFromMappings (QFiber& f) {
QMap* map = f.vm.construct<QMap>(f.vm);
f.returnValue(map);
vector<QV, trace_allocator<QV>> pairs(f.vm), tuple(f.vm);
auto citr = copyVisitor(std::back_inserter(pairs)), citr2 = copyVisitor(std::back_inserter(tuple));
for (int i=1, l=f.getArgCount(); i<l; i++) {
pairs.clear();
f.at(i).copyInto(f, citr);
for (QV& pair: pairs) {
tuple.clear();
pair.copyInto(f, citr2);
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

static void mapSubscript (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
f.returnValue(map.get(f.at(1)));
}

static void mapSubscriptSetter (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
f.returnValue(map .set(f.at(1), f.at(2))); 
}

static void mapLength (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
f.returnValue(static_cast<double>(map.map.size())); 
}

static void mapClear (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
map.map.clear();
}

static void mapReserve (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
map.map.reserve(f.getNum(1)); 
}

void QVM::initMapType () {
mappingClass
->copyParentMethods()
->assoc<QObject>();

mapClass
->copyParentMethods()
->bind("[]", mapSubscript)
->bind("[]=", mapSubscriptSetter)
->bind("in", mapIn)
->bind("length", mapLength)
->bind("toString", mapToString)
->bind("iterator", mapIterator)
->bind("clear", mapClear)
->bind("remove", mapRemove)
->bind("reserve", mapReserve)
->assoc<QMap>();

mapIteratorClass
->copyParentMethods()
->bind("next", mapIteratorNext)
->assoc<QMapIterator>();

mappingClass->type
->copyParentMethods()
->assoc<QClass>();

mapClass ->type
->copyParentMethods()
->bind("()", mapInstantiateFromMappings)
->bind("of", mapInstantiateFromEntries)
->assoc<QClass>();

//println("sizeof(QMap)=%d", sizeof(QMap));
}
