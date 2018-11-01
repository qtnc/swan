#include "QValue.hpp"
#include "QValueExt.hpp"
#include "../include/cpprintf.hpp"
#include<fstream>
#include<cmath>
#include<cstdlib>
#include<boost/algorithm/string.hpp>
#include<boost/regex.hpp>
#include<utf8.h>
using namespace std;

template<class T> void unordered_set_union (const T& set1, const T& set2, T& result) {
for (auto& val: set1) result.insert(val);
for (auto& val: set2) result.insert(val);
}

template<class T> unordered_set_intersection (const T& set1, const T& set2, T& result) {
for (auto& val: set1) if (set2.find(val)!=set2.end()) result.insert(val);
}

template<class T> unordered_set_difference  (const T& set1, const T& set2, T& result) {
for (auto& val: set1) if (set2.find(val)==set2.end()) result.insert(val);
}

template<class T> unordered_set_symetric_difference  (const T& set1, const T& set2, T& result) {
T intersection, union_;
unordered_set_union(set1, set2, union_);
unordered_set_intersection(set1, set2, intersection);
unordered_set_difference(union_, intersection, result);
}

template <class T> T clamp (T min, T val, T max) {
return val<min? min : (val>max? max : val);
}

template<double(*F)(double)> static void numMathFunc (QFiber& f) {
f.returnValue(F(f.getNum(0)));
}

double nativeClock ();

static void doNothing (QFiber& f) { }

static void fiberInstantiate (QFiber& f) {
QClosure& closure = f.getObject<QClosure>(1);
QFiber* fb = new QFiber(f.vm, closure);
f.returnValue(QV(fb, QV_TAG_FIBER));
}

static void fiberNext (QFiber& f) {
f.callFiber(f.getObject<QFiber>(0), f.getArgCount() -1); 
f.returnValue(f.at(1)); 
}

static void listInstantiate (QFiber& f) {
int n = f.getArgCount() -1;
QList* list = new QList(f.vm);
if (n>0) list->data.insert(list->data.end(), &f.at(1), &f.at(1) +n);
f.returnValue(list);
}

static void listIterate (QFiber& f) {
QList& list = f.getObject<QList>(0);
int i = 1 + (f.isNull(1)? -1 : f.getNum(1));
f.returnValue(i>=list.data.size()? QV() : QV(static_cast<double>(i)));
}

static void listSubscript (QFiber& f) {
QList& list = f.getObject<QList>(0);
int length = list.data.size();
if (f.isNum(1)) {
int i = f.getNum(1);
if (i<0) i+=length;
f.returnValue(i>=0 && i<length? list.data.at(i) : QV());
}
else if (f.isRange(1)) {
int start, end;
f.getRange(1).makeBounds(length, start, end);
QList* newList = new QList(f.vm);
if (end-start>0) newList->data.insert(newList->data.end(), list.data.begin()+start, list.data.begin()+end);
f.returnValue(newList);
}
else f.returnValue(QV());
}

static void listSubscriptSetter (QFiber& f) {
QList& list = f.getObject<QList>(0);
int length = list.data.size();
if (f.isNum(1)) {
int i = f.getNum(1);
if (i<0) i+=length;
f.returnValue(list.data.at(i) = f.at(2));
}
else if (f.isRange(1)) {
int start, end;
f.getRange(1).makeBounds(length, start, end);
list.data.erase(list.data.begin()+start, list.data.begin()+end);
f.getObject<QSequence>(2).insertIntoVector(f, list.data, start);
f.returnValue(f.at(2));
}
else f.returnValue(QV());
}

static void listPush (QFiber& f) {
QList& list = f.getObject<QList>(0);
int n = f.getArgCount() -1;
if (n>0) list.data.insert(list.data.end(), &f.at(1), (&f.at(1))+n);
}

static void listPop (QFiber& f) {
QList& list = f.getObject<QList>(0);
if (list.data.empty()) f.returnValue(QV());
else {
f.returnValue(list.data.back());
list.data.pop_back();
}}

static void listInsert (QFiber& f) {
QList& list = f.getObject<QList>(0);
int n = f.getNum(1), count = f.getArgCount() -2;
auto pos = n>=0? list.data.begin()+n : list.data.end()+n;
list.data.insert(pos, &f.at(2), (&f.at(2))+count);
}

static void listRemoveAt (QFiber& f) {
QList& list = f.getObject<QList>(0);
for (int i=1, l=f.getArgCount(); i<l; i++) {
if (f.isNum(i)) {
int n = f.getNum(i);
auto pos = n>=0? list.data.begin()+n : list.data.end()+n;
list.data.erase(pos);
}
else if (f.isRange(i)) {
int start, end;
f.getRange(i).makeBounds(list.data.size(), start, end);
list.data.erase(list.data.begin()+start, list.data.begin()+end);
}
}}

static void listRemove (QFiber& f) {
QList& list = f.getObject<QList>(0);
QVEqualler eq;
for (int i=1, l=f.getArgCount(); i<l; i++) {
QV& toRemove = f.at(i);
auto it = find_if(list.data.begin(), list.data.end(), [&](const QV& v){ return eq(v, toRemove); });
if (it!=list.data.end()) {
f.returnValue(*it);
list.data.erase(it);
}
else f.returnValue(QV());
}}

static void listRemoveIf (QFiber& f) {
QList& list = f.getObject<QList>(0);
for (int i=1, l=f.getArgCount(); i<l; i++) {
QVUnaryPredicate pred(f.at(i));
auto it = remove_if(list.data.begin(), list.data.end(), pred);
list.data.erase(it, list.data.end());
}}

static void listIndexOf (QFiber& f) {
QList& list = f.getObject<QList>(0);
QV& needle = f.at(1);
int start = f.getOptionalNum(2, 0);
QVEqualler eq;
auto end = list.data.end(), begin = start>=0? list.data.begin()+start : list.data.end()+start,
re = find_if(begin, end, [&](const QV& v){ return eq(v, needle); });
f.returnValue(re==end? -1 : re-list.data.begin());
}

static void listLastIndexOf (QFiber& f) {
QList& list = f.getObject<QList>(0);
QV& needle = f.at(1);
int start = f.getOptionalNum(2, list.data.size());
auto begin = list.data.begin(), end = start>=0? list.data.begin()+start : list.data.end()+start,
re = find_end(begin, end, &needle, (&needle)+1, QVEqualler());
f.returnValue(re==end? -1 : re-list.data.begin());
}

static void listSort (QFiber& f) {
QList& list = f.getObject<QList>(0);
if (f.getArgCount()>=2) sort(list.data.begin(), list.data.end(), QVBinaryPredicate(f.at(1)));
else sort(list.data.begin(), list.data.end(), QVLess());
}

static void listReverse (QFiber& f) {
QList& list = f.getObject<QList>(0);
reverse(list.data.begin(), list.data.end());
}

static void listRotate (QFiber& f) {
QList& list = f.getObject<QList>(0);
int offset = f.getNum(1);
auto middle = offset>=0? list.data.begin()+offset : list.data.end()+offset;
rotate(list.data.begin(), middle, list.data.end());
}

static void listFromSequence (QFiber& f) {
QList* list = new QList(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i).insertIntoVector(f, list->data, list->data.size());
}
f.returnValue(list);
}

static void listToString (QFiber& f) {
QList& list = f.getObject<QList>(0);
string re = "[";
list.join(f, ", ", re);
re += "]";
f.returnValue(re);
}

static void mapIn (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
auto it = map.map.find(f.at(1));
f.returnValue(it!=map.map.end());
}

static void mapInstantiate (QFiber& f) {
QMap* map = new QMap(f.vm);
vector<QV> tuple;
for (int i=1, l=f.getArgCount(); i<l; i++) {
tuple.clear();
f.getObject<QSequence>(i).insertIntoVector(f, tuple, 0);
map->map[tuple[0]] = tuple.back();
}
f.returnValue(map);
}

static void mapFromSequence (QFiber& f) {
QMap* map = new QMap(f.vm);
vector<QV> pairs, tuple;
for (int i=1, l=f.getArgCount(); i<l; i++) {
pairs.clear();
f.getObject<QSequence>(i).insertIntoVector(f, pairs, 0);
for (QV& pair: pairs) {
tuple.clear();
pair.asObject<QSequence>()->insertIntoVector(f, tuple, 0);
map->map[tuple[0]] = tuple.back();
}}
f.returnValue(map);
}

static void mapIterate (QFiber& f) {
QMap& map = f.getObject<QMap>(0);
if (f.isNull(1)) {
f.returnValue(new QMapIterator(f.vm, map));
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
mi.iterator++;
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

static QV stringToNumImpl (QString& s, int base) {
char* end = nullptr;
if (base<0 || base>32) {
double re = strtod(s.begin(), &end);
return end && end==s.end()? re : QV();
} else {
long long re = strtoll(s.begin(), &end, base);
return end && end==s.end()? static_cast<double>(re) : QV();
}}

static void numInstantiate (QFiber& f) {
QV& val = f.at(1);
if (val.isNum()) f.returnValue(val);
else if (val.isBool()) f.returnValue(val.asBool()? 1 : 0);
else if (val.isString()) {
int base = f.getOptionalNum(2, -1);
f.returnValue(stringToNumImpl(f.getObject<QString>(1), base));
}
else f.returnValue(QV());
}

static void numLog (QFiber& f) {
double d = f.getNum(0), base = f.getOptionalNum(1, -1);
f.returnValue(base>0? log(d)/log(base) : log(d));
}

static void numToString (QFiber& f) {
double val = f.getNum(0);
if (isnan(val)) f.returnValue(QV(f.vm, "NaN", 3));
else if (isinf(val)) f.returnValue(QV(f.vm, "infinity", 8));
else f.returnValue(QV(f.vm, format("%.14g", val) ));
}

static void objectInstantiate (QFiber& f) {
QClass& cls = f.getObject<QClass>(0);
QObject* instance = cls.instantiate();
f.setObject(0, instance);
f.pushCppCallFrame();
f.callSymbol(f.vm.findMethodSymbol("constructor"), f.getArgCount());
f.popCppCallFrame();
f.returnValue(instance);
}

static void objectHashCode (QFiber& f) {
uint32_t h = FNV_OFFSET, *u = reinterpret_cast<uint32_t*>(&(f.at(0).i));
h ^= u[0] ^u[1];
f.returnValue(static_cast<double>(h));
}

static void objectToString (QFiber& f) {
const QClass& cls = f.at(0).getClass(f.vm);
f.returnValue(format("%s@%#0$16llX", cls.name, f.at(0).i) );
}

static void rangeInstantiate (QFiber& f) {
int nArgs = f.getArgCount();
double start=0, end=f.getNum(1), step=1;
bool inclusive = f.getOptionalBool(nArgs -1, false);
if (nArgs>2 && f.isNum(2)) {
start = end;
end = f.getNum(2);
}
if (nArgs>3 && f.isNum(3)) step = f.getNum(3);
else step = end>=start? 1 : -1;
f.returnValue(new QRange(f.vm, start, end, step, inclusive));
}

static void rangeIn (QFiber& f) {
QRange& r = f.getObject<QRange>(0);
double x = f.getNum(1);
bool re;
if (r.step>0 && (x<r.start || x>r.end)) re = false;
else if (r.step<0 && (x>r.start || x<r.end)) re = false;
else if (x==r.end) re=r.inclusive;
else re = 0==fmod(x-r.start, r.step);
f.returnValue(re);
}

static inline QV rangeMake (QVM& vm, double start, double end, bool inclusive) {
double step = end>=start? 1 : -1;
return new QRange(vm, start, end, step, inclusive);
}

static void rangeToString (QFiber& f) {
QRange& r = f.getObject<QRange>(0);
if (r.step==1 || r.step==-1) f.returnValue(format("%g%s%g", r.start, r.inclusive?"...":"..", r.end));
else f.returnValue(format("Range(%g, %g, %g, %b)", r.start, r.end, r.step, r.inclusive));
}

static void sequenceJoin (QFiber& f) {
QSequence& seq = f.getObject<QSequence>(0);
string out, delim = f.getOptionalString(1, "");
seq.join(f, delim, out);
f.returnValue(out);
}

static void setInstantiate (QFiber& f) {
int n = f.getArgCount() -1;
QSet* set = new QSet(f.vm);
if (n>0) set->set.insert(&f.at(1), &f.at(1) +n);
f.returnValue(set);
}

static void setFromSequence (QFiber& f) {
QSet* set = new QSet(f.vm);
for (int i=1, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i).insertIntoSet(f, *set);
}
f.returnValue(set);
}

static void setAdd (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
int n = f.getArgCount() -1;
if (n>0) set.set.insert(&f.at(1), (&f.at(1))+n);
}

static void setIn (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
auto it = set.set.find(f.at(1));
f.returnValue(it!=set.set.end());
}

static void setUnion (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = new QSet(f.vm);
unordered_set_union(set1.set, set2.set, result->set);
f.returnValue(result);
}

static void setIntersection (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = new QSet(f.vm);
unordered_set_intersection(set1.set, set2.set, result->set);
f.returnValue(result);
}

static void setDifference (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = new QSet(f.vm);
unordered_set_difference(set1.set, set2.set, result->set);
f.returnValue(result);
}

static void setSymetricDifference (QFiber& f) {
QSet &set1 = f.getObject<QSet>(0), &set2 = f.getObject<QSet>(1);
QSet* result = new QSet(f.vm);
unordered_set_symetric_difference(set1.set, set2.set, result->set);
f.returnValue(result);
}

static void setRemove (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
for (int i=1, n=f.getArgCount(); i<n; i++) {
auto it = set.set.find(f.at(i));
if (it==set.set.end()) f.returnValue(QV());
else {
f.returnValue(*it);
set.set.erase(it);
}}}

static void setIterate (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
if (f.isNull(1)) {
f.returnValue(new QSetIterator(f.vm, set));
}
else {
QSetIterator& mi = f.getObject<QSetIterator>(1);
bool cont = mi.iterator != set.set.end();
f.returnValue( cont? f.at(1) : QV());
}}

static void setIteratorValue (QFiber& f) {
QSetIterator& mi = f.getObject<QSetIterator>(1);
QV val = *mi.iterator++;
f.returnValue(val);
}

static void setToString (QFiber& f) {
QSet& set = f.getObject<QSet>(0);
string re = "<";
set.join(f, ", ", re);
re += ">";
f.returnValue(re);
}

static int stringCompare (QFiber& f) {
QString& s1 = f.getObject<QString>(0);
QString& s2 = *f.ensureString(1);
return strnatcmp(s1.data, s2.data);
}

static void stringIterate (QFiber& f) {
QString& s = f.getObject<QString>(0);
int i = 1 + (f.isNull(1)? -1 : f.getNum(1)), length = utf8::distance(s.begin(), s.end());
f.returnValue(i>=length? QV() : QV(static_cast<double>(i)));
}

static void stringHashCode (QFiber& f) {
QString& s = f.getObject<QString>(0);
size_t re = hashBytes(reinterpret_cast<uint8_t*>(s.begin()), reinterpret_cast<uint8_t*>(s.end()));
f.returnValue(static_cast<double>(re));
}

static void stringPlus (QFiber& f) {
QString &first = f.getObject<QString>(0), &second = *f.ensureString(1);
uint32_t length = first.length + second.length;
QString* result = newVLS<QString, char>(length+1, f.vm, length);
memcpy(result->data, first.data, first.length);
memcpy(result->data + first.length, second.data, second.length);
result->data[length] = 0;
f.returnValue(result);
}

static void stringTimes (QFiber& f) {
QString& s = f.getObject<QString>(0);
int times = f.getNum(1);
if (times<0) f.returnValue(QV());
else if (times==0) f.returnValue(QString::create(f.vm, nullptr, 0));
else if (times==1) f.returnValue(&s);
else {
uint32_t length = times * s.length;
QString* result = newVLS<QString, char>(length+1, f.vm, length);
for (int i=0; i<times; i++) memcpy(result->data+s.length*i, s.data, s.length);
result->data[length] = 0;
f.returnValue(result);
}}

static void stringLength (QFiber& f) {
QString& s = f.getObject<QString>(0);
f.returnValue(utf8::distance(s.data, s.data+s.length));
}

static void stringFromSequence (QFiber& f) {
vector<QV> values;
for (int i=1, l=f.getArgCount(); i<l; i++) {
if (f.isNum(i) || f.isString(i)) values.push_back(f.at(i));
else f.getObject<QSequence>(i).insertIntoVector(f, values, values.size());
}
string re;
auto out = back_inserter(re);
for (auto& val: values) {
if (val.isString()) {
QString& s = *val.asObject<QString>();
re.insert(re.end(), s.begin(), s.end());
}
else if (val.isNum()) utf8::append(val.asNum(), out);
else {
QString& s = *f.ensureString(val);
re.insert(re.end(), s.begin(), s.end());
}}
f.returnValue(re);
}

static void stringCodePointAt (QFiber& f) {
QString& s = f.getObject<QString>(0);
int i = f.getNum(1), length = utf8::distance(s.begin(), s.end());
if (i<0) i+=length;
if (i<0 || i>=length) { f.returnValue(QV()); return; }
auto it = s.begin();
utf8::advance(it, i, s.end());
f.returnValue( static_cast<double>(utf8::next(it, s.end()) ));
}

static void stringSubscript (QFiber& f) {
QString& s = f.getObject<QString>(0);
int start=0, end=0, length = utf8::distance(s.data, s.data+s.length);
if (f.isNum(1)) {
start = f.getNum(1);
if (start<0) start+=length;
if (start>=length) { f.returnValue(QV()); return; }
end = start +1;
}
else if (f.isRange(1)) {
f.getRange(1).makeBounds(length, start, end);
}
auto startPos = s.begin();
utf8::advance(startPos, start, s.end());
auto endPos = startPos;
utf8::advance(endPos, end-start, s.end());
f.returnValue(QV( QString::create(f.vm, startPos, endPos), QV_TAG_STRING));
}

static void stringFind (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
int start = f.getOptionalNum(2, 0), length = utf8::distance(s->begin(), s->end());
if (start<0) start += length;
auto endPos = s->end(), startPos = s->begin();
utf8::advance(startPos, start, endPos);
auto re = search(startPos, endPos, needle->begin(), needle->end());
if (re==endPos) f.returnValue(-1);
else f.returnValue(utf8::distance(s->begin(), re));
}

static void stringRfind (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
int length = utf8::distance(s->begin(), s->end());
int end = f.getOptionalNum(2, length);
if (end<0) end += length;
auto startPos = s->begin(), endPos = startPos;
utf8::advance(endPos, end, s->end());
auto re = find_end(startPos, endPos, needle->begin(), needle->end());
if (re==endPos) f.returnValue(-1);
else f.returnValue(utf8::distance(s->begin(), re));
}

static void stringFindFirstOf (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
int start = f.getOptionalNum(2, 0), length = utf8::distance(s->begin(), s->end());
if (start<0) start += length;
auto endPos = s->end(), startPos = s->begin();
utf8::advance(startPos, start, endPos);
auto re = find_first_of(startPos, endPos, needle->begin(), needle->end());
if (re==endPos) f.returnValue(-1);
else f.returnValue(utf8::distance(s->begin(), re));
}

static void stringIn (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
auto it = search(s->begin(), s->end(), needle->begin(), needle->end());
f.returnValue(it!=s->end());
}

static void stringStartsWith (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
if (needle->length>s->length) { f.returnValue(false); return; }
else f.returnValue(equal(needle->begin(), needle->end(), s->begin()));
}

static void stringEndsWith (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
if (needle->length>s->length) { f.returnValue(false); return; }
else f.returnValue(equal(needle->begin(), needle->end(), s->end() - needle->length));
}

static void stringUpper (QFiber& f) {
QString& s = f.getObject<QString>(0);
string re;
re.reserve(s.length);
auto out = back_inserter(re);
for (utf8::iterator<char*> it(s.begin(), s.begin(), s.end()), end(s.end(), s.begin(), s.end()); it!=end; ++it) utf8::append(toupper(*it), out);
f.returnValue(re);
}

static void stringLower (QFiber& f) {
QString& s = f.getObject<QString>(0);
string re;
re.reserve(s.length);
auto out = back_inserter(re);
for (utf8::iterator<char*> it(s.begin(), s.begin(), s.end()), end(s.end(), s.begin(), s.end()); it!=end; ++it) utf8::append(tolower(*it), out);
f.returnValue(re);
}

static void stringToNum (QFiber& f) {
QString& s = f.getObject<QString>(0);
int base = f.getOptionalNum(1, -1);
f.returnValue(stringToNumImpl(s, base));
}

static void tupleInstantiate (QFiber& f) {
int n = f.getArgCount() -1;
QTuple* tuple = QTuple::create(f.vm, n, n? &f.at(1) : nullptr);
f.returnValue(tuple);
}

static void tupleFromSequence (QFiber& f) {
vector<QV> items;
for (int i=1, l=f.getArgCount(); i<l; i++) {
f.getObject<QSequence>(i).insertIntoVector(f, items, items.size());
}
f.returnValue(QTuple::create(f.vm, items.size(), &items[0]));
}

static void tupleIterate (QFiber& f) {
QTuple& tuple = f.getObject<QTuple>(0);
int i = 1 + f.getOptionalNum(1, -1);
f.returnValue(i>=tuple.length? QV() : QV(static_cast<double>(i)));
}

static void tupleSubscript (QFiber& f) {
QTuple& tuple = f.getObject<QTuple>(0);
if (f.isNum(1)) {
int i = f.getNum(1);
if (i<0) i+=tuple.length;
f.returnValue(i>=0 && i<tuple.length? tuple.at(i) : QV());
}
else if (f.isRange(1)) {
int start, end;
f.getRange(1).makeBounds(tuple.length, start, end);
QTuple* newTuple = QTuple::create(f.vm, end-start, tuple.data+start);
f.returnValue(newTuple);
}
else f.returnValue(QV());
}


static void tupleToString (QFiber& f) {
QTuple& tuple = f.getObject<QTuple>(0);
string re = "(";
tuple.join(f, ", ", re);
if (tuple.length==1) re+=",";
re += ")";
f.returnValue(re);
}

static void tupleHashCode (QFiber& f) {
QTuple& t = f.getObject<QTuple>(0);
int hashCodeSymbol = f.vm.findMethodSymbol("hashCode");
size_t re = FNV_OFFSET ^t.length;
for (uint32_t i=0, n=t.length; i<n; i++) {
f.pushCppCallFrame();
f.push(t.data[i]);
f.callSymbol(hashCodeSymbol, 1);
size_t h = static_cast<size_t>(f.at(-1).d);
f.pop();
f.popCppCallFrame();
re = (re^h) * FNV_PRIME;
}
f.returnValue(static_cast<double>(re));
}

static void bufferIterate (QFiber& f) {
QBuffer& b = f.getObject<QBuffer>(0);
int i = 1 + (f.isNull(1)? -1 : f.getNum(1));
f.returnValue(i>=b.length? QV() : QV(static_cast<double>(i)));
}

static void bufferHashCode (QFiber& f) {
QBuffer& b = f.getObject<QBuffer>(0);
size_t re = hashBytes(b.begin(), b.end());
f.returnValue(static_cast<double>(re));
}

static void bufferPlus (QFiber& f) {
QBuffer &first = f.getObject<QBuffer>(0), &second = f.getObject<QBuffer>(1);
uint32_t length = first.length + second.length;
QBuffer* result = newVLS<QBuffer, uint8_t>(length+1, f.vm, length);
memcpy(result->data, first.data, first.length);
memcpy(result->data + first.length, second.data, second.length);
result->data[length] = 0;
f.returnValue(result);
}

static void bufferFromSequence (QFiber& f) {
vector<QV> values;
for (int i=1, l=f.getArgCount(); i<l; i++) {
if (f.isNum(i) || f.isBuffer(i)) values.push_back(f.at(i));
else f.getObject<QSequence>(i).insertIntoVector(f, values, values.size());
}
string re;
auto out = back_inserter(re);
for (auto& val: values) {
if (val.isInstanceOf(f.vm.bufferClass)) {
QBuffer& b = *val.asObject<QBuffer>();
re.insert(re.end(), reinterpret_cast<const char*>(b.begin()), reinterpret_cast<const char*>(b.end()));
}
else if (val.isNum()) *out++ = static_cast<char>(val.asNum());
}
QBuffer* b = QBuffer::create(f.vm, reinterpret_cast<const uint8_t*>(re.data()), re.size());
f.returnValue(b);
}

static void bufferSubscript (QFiber& f) {
QBuffer& b = f.getObject<QBuffer>(0);
if (f.isNum(1)) {
int pos = f.getNum(1);
if (pos<0) pos+=b.length;
f.returnValue(pos<0||pos>=b.length? QV() : QV(static_cast<double>(b.data[pos])));
}
else if (f.isRange(1)) {
int start=0, end=0;
f.getRange(1).makeBounds(b.length, start, end);
f.returnValue(QBuffer::create(f.vm, &b.data[start], &b.data[end]));
}}

static void bufferFind (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
int start = f.getOptionalNum(2, 0);
if (start<0) start += b.length;
auto endPos = b.end(), startPos = b.begin()+start;
auto re = search(startPos, endPos, needle.begin(), needle.end());
if (re==endPos) f.returnValue(-1);
else f.returnValue(static_cast<double>(re - b.begin()));
}

static void bufferRfind (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
int end = f.getOptionalNum(2, b.length);
if (end<0) end += b.length;
auto startPos = b.begin(), endPos = b.begin()+end;
auto re = find_end(startPos, endPos, needle.begin(), needle.end());
if (re==endPos) f.returnValue(-1);
else f.returnValue(static_cast<double>(re - b.begin()));
}

static void bufferFindFirstOf (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
int start = f.getOptionalNum(2, 0);
if (start<0) start += b.length;
auto endPos = b.end(), startPos = b.begin()+start;
auto re = find_first_of(startPos, endPos, needle.begin(), needle.end());
if (re==endPos) f.returnValue(-1);
else f.returnValue(static_cast<double>(re - b.begin()));
}

static void bufferIn (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
auto it = search(b.begin(), b.end(), needle.begin(), needle.end());
f.returnValue(it!=b.end());
}

static void bufferStartsWith (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
if (needle.length>b.length) { f.returnValue(false); return; }
else f.returnValue(equal(needle.begin(), needle.end(), b.begin()));
}

static void bufferEndsWith (QFiber& f) {
QBuffer &b = f.getObject<QBuffer>(0), &needle = f.getObject<QBuffer>(1);
if (needle.length>b.length) { f.returnValue(false); return; }
else f.returnValue(equal(needle.begin(), needle.end(), b.end() - needle.length));
}

static string normalizeEncodingName (const string& name) {
string enc = boost::to_lower_copy(name);
auto it = remove_if(enc.begin(), enc.end(), boost::is_any_of("-_"));
enc.erase(it, enc.end());
return enc;
}

static QString* convertBufferToString (QBuffer& b, const string& encoding) {
auto it = QVM::bufferToStringConverters.find(normalizeEncodingName(encoding));
if (it==QVM::bufferToStringConverters.end()) throw std::logic_error(format("No converter found to convert from %s to %s", encoding, "UTF-8"));
string re = (it->second)( reinterpret_cast<const char*>(b.begin()), reinterpret_cast<const char*>(b.end()) );
return QString::create(b.type->vm, re);
}

static QBuffer* convertStringToBuffer (QString& s, const string& encoding) {
auto it = QVM::stringToBufferConverters.find(normalizeEncodingName(encoding));
if (it==QVM::stringToBufferConverters.end()) throw std::logic_error(format("No converter found to convert from %s to %s", "UTF-8", encoding));
string re = (it->second)( s.begin(), s.end() );
return QBuffer::create(s.type->vm, &re[0], re.size());
}

static void bufferInstantiate (QFiber& f) {
QString* s = f.ensureString(1);
string enc = f.getOptionalString(2, "UTF-8");
f.returnValue(convertStringToBuffer(*s, enc));
}

static void stringInstantiate (QFiber& f) {
if (f.isBuffer(1)) {
QBuffer& b = f.getObject<QBuffer>(1);
QString* enc = f.ensureString(2);
f.returnValue(convertBufferToString(b, enc->asString()));
}
//todo
}

static void stringToBuffer (QFiber& f) {
QString &s = f.getObject<QString>(0), *enc = f.ensureString(1);
f.returnValue(convertStringToBuffer(s, enc->asString()));
}

static void bufferToString (QFiber& f) {
QBuffer& b = f.getObject<QBuffer>(0);
if (f.getArgCount()>=2) {
QString* enc = f.ensureString(1);
f.returnValue(convertBufferToString(b, enc->asString()));
} 
else {
string out = "Buffer:";
for (uint8_t x: b) {
if (x<32 || x>=127) out += format("%%%0$2X", static_cast<int>(x));
else out += static_cast<char>(x);
}
f.returnValue(QV(QString::create(f.vm, out), QV_TAG_STRING));
}}

static void regexInstantiate (QFiber& f) {
QString &pattern = f.getObject<QString>(1);
auto opt = QRegex::parseOptions(f.getArgCount()>2 && f.isString(2)? f.getObject<QString>(2).begin() : nullptr);
QRegex* regex = new QRegex(f.vm, pattern.begin(), pattern.end(), opt.first, opt.second);
f.returnValue(regex);
}

static void regexIteratorIterate (QFiber& f) {
QRegexIterator& r = f.getObject<QRegexIterator>(0);
if (r.it==r.end) f.returnValue(QV());
}

static void regexTokenIteratorIterate (QFiber& f) {
QRegexTokenIterator& r = f.getObject<QRegexTokenIterator>(0);
if (r.it==r.end) f.returnValue(QV());
}

static void regexIteratorValue (QFiber& f) {
QRegexIterator& r = f.getObject<QRegexIterator>(0);
QRegexMatchResult* mr = new QRegexMatchResult(f.vm);
mr->match = *r.it++;
f.returnValue(mr);
}

static void regexTokenIteratorValue (QFiber& f) {
QRegexTokenIterator& r = f.getObject<QRegexTokenIterator>(0);
f.returnValue(*r.it++);
}

static void regexMatchResultStart (QFiber& f) {
QRegexMatchResult& m = f.getObject<QRegexMatchResult>(0);
if (f.isString(1)) f.returnValue(static_cast<double>(m.match.position(f.getObject<QString>(1).begin())));
else f.returnValue(static_cast<double>(m.match.position(f.getOptionalNum(1, 0))));
}

static void regexMatchResultLength (QFiber& f) {
QRegexMatchResult& m = f.getObject<QRegexMatchResult>(0);
if (f.isString(1)) f.returnValue(static_cast<double>(m.match.length(f.getObject<QString>(1).begin())));
else f.returnValue(static_cast<double>(m.match.length(f.getOptionalNum(1, 0))));
}

static void regexMatchResultEnd (QFiber& f) {
QRegexMatchResult& m = f.getObject<QRegexMatchResult>(0);
if (f.isString(1)) {
auto i = f.getObject<QString>(1).begin();
f.returnValue(static_cast<double>(m.match.position(i) + m.match.length(i)));
}
else {
int i = f.getOptionalNum(1, 0);
f.returnValue(static_cast<double>(m.match.position(i) + m.match.length(i)));
}}

static void regexMatchResultSubscript (QFiber& f) {
QRegexMatchResult& m = f.getObject<QRegexMatchResult>(0);
auto& sub =  f.isString(1)?
m.match[ f.getObject<QString>(1).begin() ]:
m.match[ f.getOptionalNum(1, 0) ];
f.returnValue(QV(QString::create(f.vm, sub.first, sub.second), QV_TAG_STRING));
}

static void regexTest (QFiber& f) {
QRegex& r = f.getObject<QRegex>(0);
QString& s = f.getObject<QString>(1);
boost::cmatch unused;
f.returnValue(boost::regex_match(const_cast<const char*>(s.begin()), const_cast<const char*>(s.end()), unused, r.regex, r.matchOptions | boost::regex_constants::match_nosubs | boost::regex_constants::match_any));
}

static inline QRegex& ensureRegex (QFiber& f, int i) {
if (f.isString(i)) {
QString& s = f.getObject<QString>(i);
return *new QRegex(f.vm, s.begin(), s.end(), boost::regex::literal, boost::regex_constants::format_literal);
}
else return f.getObject<QRegex>(i);
}

static void stringSearch (QFiber& f) {
QString& s = f.getObject<QString>(0);
QRegex& r = ensureRegex(f,1);
int re=-1, start = f.getOptionalNum(2, 0);
bool full = f.getOptionalBool(2, false) || f.getOptionalBool(3, false);
auto options = full? boost::regex_constants::match_default : boost::regex_constants::match_nosubs;
boost::cmatch match;
if (start<0) start+=s.length;
if (start>0) options |= boost::regex_constants::match_prev_avail;
if (boost::regex_search(const_cast<const char*>(s.begin()+start), const_cast<const char*>(s.end()), match, r.regex, r.matchOptions | options)) re = match.position()+start;
if (full && re<0) f.returnValue(QV());
else if (full) f.returnValue(new QRegexMatchResult(f.vm, match));
else f.returnValue(static_cast<double>(re));
}

static void stringFindAll (QFiber& f) {
QString& s = f.getObject<QString>(0);
QRegex& r = ensureRegex(f,1);
int group = f.getOptionalNum(2, -2);
bool full = f.getOptionalBool(2, false);
if (full) f.returnValue(new QRegexIterator(f.vm, s, r, r.matchOptions));
else {
int groupCount = r.regex.mark_count();
if (group<-1 || group>groupCount) group = groupCount==0?0:1;
f.returnValue(new QRegexTokenIterator(f.vm, s, r, r.matchOptions, group));
}}

static void stringSplit (QFiber& f) {
QString& s = f.getObject<QString>(0);
QRegex& r = ensureRegex(f,1);
f.returnValue(new QRegexTokenIterator(f.vm, s, r, r.matchOptions, -1));
}

static void stringReplace (QFiber& f) {
QString& s = f.getObject<QString>(0);
QRegex& r = ensureRegex(f,1);
string re;
auto out = back_inserter(re);
if (f.isString(2)) {
QString& fmt = f.getObject<QString>(2);
boost::regex_replace(out, s.begin(), s.end(), r.regex, fmt.begin(), r.matchOptions);
}
else {
QV& fmt = f.at(2);
boost::regex_replace(out, const_cast<const char*>(s.begin()), const_cast<const char*>(s.end()), r.regex, [&](auto& m, auto& o){
f.push(fmt);
f.push(new QRegexMatchResult(f.vm, m));
f.call(1);
QString* repl = f.ensureString(-1);
std::copy(repl->begin(), repl->end(), o);
f.pop();
return o;
}, r.matchOptions);
}
f.returnValue(re);
}

static void instanceofOperator (QFiber& f) {
QClass& cls1 = f.getObject<QClass>(0);
QClass& cls2 = f.at(1).getClass(f.vm);
if (cls2.isSubclassOf(f.vm.classClass)) {
QClass& cls3 = f.getObject<QClass>(1);
f.returnValue(cls3.isSubclassOf(&cls1));
}
else f.returnValue(cls2.isSubclassOf(&cls1));
}

static void defaultMessageReceiver (const QS::CompilationMessage& m) {
const char* kinds[] = { "ERROR", "WARNING", "INFO" };
println(std::cerr, "%s: %s:%d:%d near '%s': %s", kinds[m.kind], m.file, m.line, m.column, m.token, m.message);
}

static string defaultPathResolver (const string& startingPath, const string& pathToResolve) {
vector<string> cur, toResolve;
boost::split(cur, startingPath, boost::is_any_of("/\\"), boost::token_compress_off);
boost::split(toResolve, pathToResolve, boost::is_any_of("/\\"), boost::token_compress_off);
if (!cur.empty()) cur.pop_back(); // drop file name
toResolve.erase(remove_if(toResolve.begin(), toResolve.end(), [](const string& s){ return s=="."; }), toResolve.end());
{ auto it = toResolve.begin();
while((it = find(toResolve.begin()+1, toResolve.end(), ".."))!=toResolve.end()) {
toResolve.erase(--it);
toResolve.erase(it);
}}
if (!toResolve.empty() && toResolve[0]=="") cur.clear();
else while (!toResolve.empty() && !cur.empty() && toResolve[0]=="..") {
toResolve.erase(toResolve.begin());
cur.pop_back();
}
cur.insert(cur.end(), toResolve.begin(), toResolve.end());
return boost::join(cur, "/");
}

static string defaultFileLoader (const string& filename) {
ifstream in(filename);
if (!in) return "";
ostringstream out;
out << in.rdbuf();
return out.str();
}

static void qsPrint (QFiber& f) {
println("%s", f.ensureString(0)->data);
f.returnValue(QV());
}

static void import_  (QFiber& f) {
string curFile = f.getString(0), requestedFile = f.getString(1);
string finalFile = f.vm.pathResolver(curFile, requestedFile);
auto it = f.vm.imports.find(finalFile);
if (it!=f.vm.imports.end()) f.returnValue(it->second);
else {
f.loadFile(finalFile);
f.call(0);
f.returnValue(f.at(-1));
}}

QVM::QVM ():
firstGCObject(nullptr),
pathResolver(defaultPathResolver),
fileLoader(defaultFileLoader),
messageReceiver(defaultMessageReceiver)
{
objectClass = QClass::create(*this, nullptr, nullptr, "Object");
classClass = QClass::create(*this, nullptr, objectClass, "Class");
bufferMetaClass = QClass::create(*this, classClass, classClass, "BufferMetaClass");
fiberMetaClass = QClass::create(*this, classClass, classClass, "FiberMetaClass");
functionMetaClass = QClass::create(*this, classClass, classClass, "FunctionMetaClass");
listMetaClass = QClass::create(*this, classClass, classClass, "ListMetaClass");
mapMetaClass = QClass::create(*this, classClass, classClass, "MapMetaClass");
numMetaClass = QClass::create(*this, classClass, classClass, "NumMetaClass");
rangeMetaClass = QClass::create(*this, classClass, classClass, "RangeMetaClass");
regexMetaClass = QClass::create(*this, classClass, classClass, "RegexMetaClass");
setMetaClass = QClass::create(*this, classClass, classClass, "SetMetaClass");
stringMetaClass = QClass::create(*this, classClass, classClass, "StringMetaClass");
tupleMetaClass = QClass::create(*this, classClass, classClass, "TupleMetaClass");
boolClass = QClass::create(*this, classClass, objectClass, "Bool");
functionClass = QClass::create(*this, functionMetaClass, objectClass, "Function");
sequenceClass = QClass::create(*this, classClass, objectClass, "Sequence");
bufferClass = QClass::create(*this, bufferMetaClass, sequenceClass, "Buffer");
fiberClass = QClass::create(*this, fiberMetaClass, sequenceClass, "Fiber");
listClass = QClass::create(*this, listMetaClass, sequenceClass, "List");
mapClass = QClass::create(*this, mapMetaClass, sequenceClass, "Map");
nullClass = QClass::create(*this, classClass, objectClass, "Null");
numClass = QClass::create(*this, numMetaClass, objectClass, "Num");
setClass = QClass::create(*this, setMetaClass, sequenceClass, "Set");
stringClass = QClass::create(*this, stringMetaClass, sequenceClass, "String");
tupleClass = QClass::create(*this, tupleMetaClass, sequenceClass, "Tuple");
rangeClass = QClass::create(*this, rangeMetaClass, sequenceClass, "Range");
regexClass = QClass::create(*this, regexMetaClass, objectClass, "Regex");
regexMatchResultClass = QClass::create(*this, classClass, objectClass, "RegexMatchResult");
regexIteratorClass = QClass::create(*this, classClass, sequenceClass, "RegexIterator");
regexTokenIteratorClass = QClass::create(*this, classClass, sequenceClass, "RegexTokenIterator");
objectClass->type = classClass->type = classClass;

#define FUNC(BODY) [](QFiber& f){ BODY }
#define BIND_L(NAME, BODY) ->bind(#NAME, FUNC(BODY))
#define BIND_GL(NAME, BODY) bindGlobal(#NAME, QV(FUNC(BODY)));
#define BIND_F(NAME, F) ->bind(#NAME, F)
#define BIND_N(NAME) BIND_F(NAME, doNothing)

objectClass
BIND_L(type, { f.returnValue(&f.at(0).getClass(f.vm)); })
BIND_F(toString, objectToString)
BIND_L(is, { f.returnValue(f.at(0).i == f.at(1).i); })
BIND_L(==, { f.returnValue(f.at(0).i == f.at(1).i); })
BIND_L(!=, { f.returnValue(f.at(0).i != f.at(1).i); })
BIND_L(!, { f.returnValue(false); })
;

classClass
->copyParentMethods()
BIND_F( (), objectInstantiate)
BIND_F(is, instanceofOperator)
BIND_L(toString, { f.returnValue(QV(f.vm, f.getObject<QClass>(0) .name)); })
BIND_F(hashCode, objectHashCode)
BIND_L(name, { f.returnValue(QV(f.vm, f.getObject<QClass>(0) .name)); })
;

functionClass
->copyParentMethods()
BIND_L( (), {  f.callMethod(f.at(0), f.getArgCount() -1);  f.returnValue(f.at(1));  })
BIND_F(hashCode, objectHashCode)
;

boolClass
->copyParentMethods()
BIND_L(!, { f.returnValue(!f.getBool(0)); })
BIND_L(toString, { f.returnValue(QV(f.vm, f.getBool(0)? "true" : "false")); })
BIND_F(hashCode, objectHashCode)
;

nullClass
->copyParentMethods()
BIND_L(!, { f.returnValue(true); })
BIND_L(toString, { f.returnValue(QV(f.vm, "null", 4)); })
BIND_F(hashCode, objectHashCode)
;

sequenceClass
->copyParentMethods()
BIND_N(iterator)
;

#define OP(O) BIND_L(O, { f.returnValue(f.getNum(0) O f.getNum(1)); })
#define OPF(O,F) BIND_L(O, { f.returnValue(F(f.getNum(0), f.getNum(1))); })
#define OPB(O) BIND_L(O, { f.returnValue(static_cast<double>(static_cast<int64_t>(f.getNum(0)) O static_cast<int64_t>(f.getNum(1)))); })
numClass
->copyParentMethods()
OP(+) OP(-) OP(*) OP(/)
OP(<) OP(>) OP(<=) OP(>=) OP(==) OP(!=)
OPF(%, fmod)
OPF(**, pow)
OPB(|) OPB(&) OPB(^) OPB(<<) OPB(>>)
BIND_L(unm, { f.returnValue(-f.getNum(0)); })
BIND_L(unp, { f.returnValue(+f.getNum(0)); })
BIND_L(~, { f.returnValue(static_cast<double>(~static_cast<int64_t>(f.getNum(0)))); })
BIND_F(toString, numToString)
BIND_F(hashCode, objectHashCode)
BIND_L(.., { f.returnValue(rangeMake(f.vm, f.getNum(0), f.getNum(1), false)); })
BIND_L(..., { f.returnValue(rangeMake(f.vm, f.getNum(0), f.getNum(1), true)); })
;
#undef OPF
#undef OPB
#undef OP

#define OP(O) BIND_L(O, { f.returnValue(stringCompare(f) O 0); })
stringClass
->copyParentMethods()
BIND_N(toString)
BIND_F(+, stringPlus)
BIND_F(in, stringIn)
BIND_F(hashCode, stringHashCode)
BIND_F(length, stringLength)
BIND_F( [], stringSubscript)
BIND_F(iterate, stringIterate)
BIND_F(iteratorValue, stringSubscript)
OP(==) OP(!=)
OP(<) OP(>) OP(<=) OP(>=)
;
#undef OP

fiberClass
->copyParentMethods()
BIND_F( (), fiberNext)
BIND_F(iterate, fiberNext)
BIND_L(iteratorValue, { f.returnValue(f.at(1)); })
;

listClass
->copyParentMethods()
BIND_F( [], listSubscript)
BIND_F( []=, listSubscriptSetter)
BIND_L( iteratorValue, { f.returnValue(f.getObject<QList>(0) .at(f.getNum(1))); })
BIND_F(iterate, listIterate)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QList>(0).data.size())); })
BIND_F(toString, listToString)
;

mapClass
->copyParentMethods()
BIND_L( [], { f.returnValue(f.getObject<QMap>(0) .get(f.at(1))); })
BIND_L( []=, { f.returnValue(f.getObject<QMap>(0) .set(f.at(1), f.at(2))); })
BIND_F(in, mapIn)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QMap>(0).map.size())); })
BIND_F(toString, mapToString)
BIND_F(iterate, mapIterate)
BIND_F(iteratorValue, mapIteratorValue)
;

rangeClass
->copyParentMethods()
BIND_L(iteratorValue, { f.returnValue(f.at(1)); })
BIND_L(iterate, { f.returnValue(f.getObject<QRange>(0) .iterate(f.at(1))); })
BIND_F(toString, rangeToString)
BIND_F(in, rangeIn)
;

tupleClass
->copyParentMethods()
BIND_F( [], tupleSubscript)
BIND_L( iteratorValue, { f.returnValue(f.getObject<QTuple>(0) .at(f.getNum(1))); })
BIND_F(iterate, tupleIterate)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QTuple>(0).length)); })
BIND_F(toString, tupleToString)
BIND_F(hashCode, tupleHashCode)
;

stringClass
BIND_F(indexOf, stringFind)
BIND_F(lastIndexOf, stringRfind)
BIND_F(findFirstOf, stringFindFirstOf)
BIND_F(startsWith, stringStartsWith)
BIND_F(endsWith, stringEndsWith)
BIND_F(upper, stringUpper)
BIND_F(lower, stringLower)
BIND_F(toNum, stringToNum)
BIND_F(codePointAt, stringCodePointAt)
BIND_L(byteLength, { f.returnValue(static_cast<double>(f.getObject<QString>(0).length)); })
BIND_F(*, stringTimes)
BIND_F(search, stringSearch)
BIND_F(split, stringSplit)
BIND_F(replace, stringReplace)
BIND_F(findAll, stringFindAll)
BIND_F(toBuffer, stringToBuffer)
;

bufferClass
->copyParentMethods()
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QBuffer>(0).length)); })
BIND_F([], bufferSubscript)
BIND_F(+, bufferPlus)
BIND_F(in, bufferIn)
BIND_F(iterate, bufferIterate)
BIND_F(indexOf, bufferFind)
BIND_F(lastIndexOf, bufferRfind)
BIND_F(findFirstOf, bufferFindFirstOf)
BIND_F(startsWith, bufferStartsWith)
BIND_F(endsWith, bufferEndsWith)
BIND_F(toString, bufferToString)
;

listClass
BIND_L(clear, { f.getObject<QList>(0).data.clear(); })
BIND_F(add, listPush)
BIND_F(remove, listRemove)
BIND_F(removeIf, listRemoveIf)
BIND_F(push, listPush)
BIND_F(pop, listPop)
BIND_F(insert, listInsert)
BIND_F(removeAt, listRemoveAt)
BIND_F(indexOf, listIndexOf)
BIND_F(lastIndexOf, listLastIndexOf)
BIND_F(sort, listSort)
BIND_F(reverse, listReverse)
BIND_F(rotate, listRotate)
;

mapClass
BIND_L(clear, { f.getObject<QMap>(0).map.clear(); })
BIND_F(remove, mapRemove)
;

setClass
->copyParentMethods()
BIND_F(&, setIntersection)
BIND_F(|, setUnion)
BIND_F(-, setDifference)
BIND_F(^, setSymetricDifference)
BIND_F(iteratorValue, setIteratorValue)
BIND_F(iterate, setIterate)
BIND_F(toString, setToString)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QSet>(0).set.size())); })
BIND_L(clear, { f.getObject<QSet>(0).set.clear(); })
BIND_F(add, setAdd)
BIND_F(remove, setRemove)
BIND_F(in, setIn)
;

sequenceClass
BIND_F(join, sequenceJoin)
;

regexClass
->copyParentMethods()
BIND_F(test, regexTest)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QRegex>(0).regex.mark_count())); })
;

regexMatchResultClass
->copyParentMethods()
BIND_F( [], regexMatchResultSubscript)
BIND_F(start, regexMatchResultStart)
BIND_F(end, regexMatchResultEnd)
BIND_F(length, regexMatchResultLength)
;

regexIteratorClass
->copyParentMethods()
BIND_F(iterate, regexIteratorIterate)
BIND_F(iteratorValue, regexIteratorValue)
;

regexTokenIteratorClass
->copyParentMethods()
BIND_F(iterate, regexTokenIteratorIterate)
BIND_F(iteratorValue, regexTokenIteratorValue)
;

numMetaClass
->copyParentMethods()
BIND_F( (), numInstantiate)
;

stringMetaClass
->copyParentMethods()
BIND_F( (), stringInstantiate)
BIND_F( of, stringFromSequence)
;

bufferMetaClass
->copyParentMethods()
BIND_F( (), bufferInstantiate)
BIND_F(of, bufferFromSequence)
;

listMetaClass
->copyParentMethods()
BIND_F( (), listInstantiate)
BIND_F(of, listFromSequence)
;

mapMetaClass
->copyParentMethods()
BIND_F( (), mapInstantiate)
BIND_F(of, mapFromSequence)
;

setMetaClass
->copyParentMethods()
BIND_F( (), setInstantiate)
BIND_F(of, setFromSequence)
;

regexMetaClass
->copyParentMethods()
BIND_F( (), regexInstantiate)
;

rangeMetaClass
->copyParentMethods()
BIND_F( (), rangeInstantiate)
;

tupleMetaClass
->copyParentMethods()
BIND_F( (), tupleInstantiate)
BIND_F(of, tupleFromSequence)
;

fiberMetaClass
->copyParentMethods()
BIND_F( (), fiberInstantiate)
;

functionMetaClass
->copyParentMethods()
;

QClass* globalClasses[] = { boolClass, bufferClass, classClass, fiberClass, functionClass, listClass, mapClass, nullClass, numClass, objectClass, rangeClass, regexClass, sequenceClass, setClass, stringClass, tupleClass };
for (auto cls: globalClasses) bindGlobal(cls->name, cls);

bindGlobal("import", import_);
bindGlobal("print", qsPrint);
bindGlobal("NaN", QV(QV_NAN));
bindGlobal("PlusInf", QV(QV_PLUS_INF));
bindGlobal("MinusInf", QV(QV_MINUS_INF));
bindGlobal("Pi", acos(-1));
BIND_GL(clock, { f.returnValue(nativeClock()); })

#define F(X) \
numClass BIND_F(X, numMathFunc<X>); \
bindGlobal(#X, numMathFunc<X>);
F(sin) F(cos) F(tan) F(asin) F(acos) F(atan)
F(sinh) F(cosh) F(tanh) F(asinh) F(acosh) F(atanh)
F(exp) F(sqrt) F(cbrt)
F(abs) F(floor) F(ceil) F(round) F(trunc)
bindGlobal("log", numLog);
numClass
BIND_F(log, numLog)
BIND_L(frac, { double unused; f.returnValue(modf(f.getNum(0), &unused)); })
BIND_L(int, { double re; modf(f.getNum(0), &re); f.returnValue(re); })
BIND_L(sign, { double d=f.getNum(0); f.returnValue(copysign(d==0?0:1,d)); })
;
#undef F

#undef BIND_L
#undef BIND_F
#undef BIND_GL
#undef BIND_N
#undef FUNC
}

