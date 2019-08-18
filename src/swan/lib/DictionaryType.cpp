#ifndef NO_OPTIONAL_COLLECTIONS
#include "SwanLib.hpp"
#include "../vm/Dictionary.hpp"
#include "../vm/Tuple.hpp"
#include "../vm/Fiber_inlines.hpp"
using namespace std;



static void dictionaryIn (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
auto it = map.map.find(f.at(1));
f.returnValue(it!=map.map.end());
}

static void dictionaryInstantiateFromEntries (QFiber& f) {
QV sorter =  QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
int start=1, finish=f.getArgCount();
if (finish>=2) {
if (f.at(1).isCallable()) { sorter=f.at(1); start++; }
else if (f.at(-1).isCallable()) { sorter=f.at(-1); finish--; }
}
QDictionary* map = f.vm.construct<QDictionary>(f.vm, sorter);
vector<QV, trace_allocator<QV>> tuple(f.vm);
f.returnValue(map);
for (int i=start, l=finish; i<l; i++) {
tuple.clear();
f.getObject<QSequence>(i) .copyInto(f, tuple);
if (tuple.size())  map->set(tuple[0], tuple.back());
}
}

static void dictionaryInstantiateFromMappings (QFiber& f) {
int start=1, finish=f.getArgCount();
QV sorter =  QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (finish>=2) {
if (f.at(1).isCallable()) { sorter=f.at(1); start++; }
else if (f.at(-1).isCallable()) { sorter=f.at(-1); finish--; }
}
QDictionary* map = f.vm.construct<QDictionary>(f.vm, sorter);
f.returnValue(map);
vector<QV, trace_allocator<QV>> pairs(f.vm), tuple(f.vm);
for (int i=start, l=finish; i<l; i++) {
pairs.clear();
f.getObject<QSequence>(i) .copyInto(f, pairs);
for (QV& pair: pairs) {
tuple.clear();
pair.asObject<QSequence>() ->copyInto(f, tuple);
if (tuple.size()) map->set(tuple[0], tuple.back());
}}
}

static void dictionaryIterator (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
auto it = f.vm.construct<QDictionaryIterator>(f.vm, map);
f.returnValue(it);
}

static void dictionaryIteratorNext (QFiber& f) {
QDictionaryIterator& mi = f.getObject<QDictionaryIterator>(0);
mi.checkVersion();
if (mi.iterator==mi.map.map.end()) f.returnValue(QV::UNDEFINED);
else {
QV data[] = { mi.iterator->first, mi.iterator->second };
QTuple* tuple = QTuple::create(f.vm, 2, data);
++mi.iterator;
f.returnValue(tuple);
mi.forward=true;
}}

static void dictionaryIteratorPrevious (QFiber& f) {
QDictionaryIterator& mi = f.getObject<QDictionaryIterator>(0);
mi.checkVersion();
if (mi.iterator==mi.map.map.begin()) f.returnValue(QV::UNDEFINED);
else {
--mi.iterator;
QV data[] = { mi.iterator->first, mi.iterator->second };
QTuple* tuple = QTuple::create(f.vm, 2, data);
f.returnValue(tuple);
mi.forward=false;
}}

static void dictionaryIteratorRemove (QFiber& f) {
QDictionaryIterator& mi = f.getObject<QDictionaryIterator>(0);
mi.checkVersion();
if (mi.forward) --mi.iterator;
f.returnValue(mi.iterator->second);
mi.map.map.erase(mi.iterator++);
mi.incrVersion();
}

static void dictionaryToString (QFiber& f) {
bool first = true;
string out;
out += '{';
for (auto& p: f.getObject<QDictionary>(0).map) {
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

static void dictionarySubscript (QFiber& f) {
QDictionary& d = f.getObject<QDictionary>(0);
auto it = d.get(f.at(1));
f.returnValue(it==d.map.end()? QV::UNDEFINED : it->second);
}

static void dictionarySubscriptSetter (QFiber& f) {
f.getObject<QDictionary>(0) .set(f.at(1), f.at(2));
f.returnValue(f.at(2));
}

static void dictionaryRemove (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) {
auto it = map.get(f.at(i));
if (it==map.map.end()) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(it->second);
map.map.erase(it);
map.incrVersion();
}}}

static void dictionaryPut (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
map.map.insert(make_pair(f.at(1), f.at(2)));
f.returnValue(f.at(2));
map.incrVersion();
}

static void dictionaryLowerBound (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
auto it = map.map.lower_bound(f.at(1));
f.returnValue(it==map.map.end()? QV::UNDEFINED : it->first);
}

static void dictionaryUpperBound (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
auto it = map.map.upper_bound(f.at(1));
f.returnValue(it==map.map.end()? QV::UNDEFINED : it->first);
}

void QVM::initDictionaryType () {
dictionaryClass
->copyParentMethods()
BIND_F( [], dictionarySubscript)
BIND_F( []=, dictionarySubscriptSetter)
BIND_F(in, dictionaryIn)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QDictionary>(0).map.size())); })
BIND_F(toString, dictionaryToString)
BIND_F(iterator, dictionaryIterator)
BIND_L(clear, { f.getObject<QDictionary>(0).map.clear(); })
BIND_F(remove, dictionaryRemove)
BIND_F(lower, dictionaryLowerBound)
BIND_F(upper, dictionaryUpperBound)
BIND_F(put, dictionaryPut)
;

dictionaryIteratorClass
->copyParentMethods()
BIND_F(next, dictionaryIteratorNext)
BIND_F(previous, dictionaryIteratorPrevious)
BIND_F(remove, dictionaryIteratorRemove)
;

dictionaryClass ->type
->copyParentMethods()
BIND_F( (), dictionaryInstantiateFromMappings)
BIND_F(of, dictionaryInstantiateFromEntries)
;

//println("sizeof(QDictionary)=%d", sizeof(QDictionary));
}
#endif
