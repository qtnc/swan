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

static void dictionaryInstantiate (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (!sorter.isCallable()) f.runtimeError("Sorter must be callable");
QDictionary* map = f.vm.construct<QDictionary>(f.vm, sorter);
vector<QV, trace_allocator<QV>> tuple(f.vm);
f.returnValue(map);
for (int i=2, l=f.getArgCount(); i<l; i++) {
tuple.clear();
f.getObject<QSequence>(i) .copyInto(f, tuple);
if (tuple.size())  map->set(tuple[0], tuple.back());
}
}

static void dictionaryFromSequence (QFiber& f) {
QV sorter = f.getArgCount()>=2? f.at(1) : QV();
if (sorter.isNull()) sorter = QV(f.vm.findMethodSymbol("<") | QV_TAG_GENERIC_SYMBOL_FUNCTION);
if (!sorter.isCallable()) f.runtimeError("Sorter must be callable");
QDictionary* map = f.vm.construct<QDictionary>(f.vm, sorter);
f.returnValue(map);
vector<QV, trace_allocator<QV>> pairs(f.vm), tuple(f.vm);
for (int i=2, l=f.getArgCount(); i<l; i++) {
pairs.clear();
f.getObject<QSequence>(i) .copyInto(f, pairs);
for (QV& pair: pairs) {
tuple.clear();
pair.asObject<QSequence>() ->copyInto(f, tuple);
if (tuple.size()) map->set(tuple[0], tuple.back());
}}
}

static void dictionaryIterate (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
if (f.isNull(1)) {
f.returnValue(f.vm.construct<QDictionaryIterator>(f.vm, map));
}
else {
QDictionaryIterator& mi = f.getObject<QDictionaryIterator>(1);
bool cont = mi.iterator != map.map.end();
f.returnValue( cont? f.at(1) : QV());
}}

static void dictionaryIteratorValue (QFiber& f) {
QDictionaryIterator& mi = f.getObject<QDictionaryIterator>(1);
QV data[] = { mi.iterator->first, mi.iterator->second };
QTuple* tuple = QTuple::create(f.vm, 2, data);
++mi.iterator;
f.returnValue(tuple);
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
f.returnValue(it==d.map.end()? QV() : it->second);
}

static void dictionarySubscriptSetter (QFiber& f) {
f.getObject<QDictionary>(0) .set(f.at(1), f.at(2));
f.returnValue(f.at(2));
}

static void dictionaryRemove (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) {
auto it = map.get(f.at(i));
if (it==map.map.end()) f.returnValue(QV());
else {
f.returnValue(it->second);
map.map.erase(it);
}}}

static void dictionaryPut (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
map.map.insert(make_pair(f.at(1), f.at(2)));
f.returnValue(f.at(2));
}

static void dictionaryLowerBound (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
auto it = map.map.lower_bound(f.at(1));
f.returnValue(it==map.map.end()? QV() : it->first);
}

static void dictionaryUpperBound (QFiber& f) {
QDictionary& map = f.getObject<QDictionary>(0);
auto it = map.map.upper_bound(f.at(1));
f.returnValue(it==map.map.end()? QV() : it->first);
}

void QVM::initDictionaryType () {
dictionaryClass
->copyParentMethods()
BIND_F( [], dictionarySubscript)
BIND_F( []=, dictionarySubscriptSetter)
BIND_F(in, dictionaryIn)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QDictionary>(0).map.size())); })
BIND_F(toString, dictionaryToString)
BIND_F(iterate, dictionaryIterate)
BIND_F(iteratorValue, dictionaryIteratorValue)
BIND_L(clear, { f.getObject<QDictionary>(0).map.clear(); })
BIND_F(remove, dictionaryRemove)
BIND_F(lower, dictionaryLowerBound)
BIND_F(upper, dictionaryUpperBound)
BIND_F(put, dictionaryPut)
;

dictionaryMetaClass
->copyParentMethods()
BIND_F( (), dictionaryInstantiate)
BIND_F(of, dictionaryFromSequence)
;

println("sizeof(QDictionary)=%d", sizeof(QDictionary));
}
#endif
