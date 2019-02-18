#include "SwanLib.hpp"
#include "../vm/Tuple.hpp"
using namespace std;

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

static void tupleEquals (QFiber& f) {
QTuple &t1 = f.getObject<QTuple>(0), &t2 = f.getObject<QTuple>(1);
if (t1.length != t2.length) { f.returnValue(false); return; }
int eqSymbol = f.vm.findMethodSymbol("==");
bool re = true;
for (uint32_t i=0, n=t1.length; re && i<n; i++) {
f.pushCppCallFrame();
f.push(t1.data[i]);
f.push(t2.data[i]);
f.callSymbol(eqSymbol, 2);
re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
}
f.returnValue(re);
}

static void tupleCompare (QFiber& f) {
QTuple &t1 = f.getObject<QTuple>(0), &t2 = f.getObject<QTuple>(1);
if (t1.length != t2.length) { f.returnValue(static_cast<double>(t1.length-t2.length)); return; }
int compareSymbol = f.vm.findMethodSymbol("compare");
double re = 0;
for (uint32_t i=0, n=t1.length; !re && i<n; i++) {
f.pushCppCallFrame();
f.push(t1.data[i]);
f.push(t2.data[i]);
f.callSymbol(compareSymbol, 2);
re = f.at(-1).asNum();
f.pop();
f.popCppCallFrame();
}
f.returnValue(re);
}

static void tupleTimes (QFiber& f) {
QTuple& t = f.getObject<QTuple>(0);
int times = f.getNum(1);
vector<QV> items;
if (times>0) for (int i=0; i<times; i++) items.insert(items.end(), &t.data[0], &t.data[t.length]);
f.returnValue(QTuple::create(f.vm, items.size(), &items[0]));
}

void QVM::initTupleType () {
tupleClass
->copyParentMethods()
BIND_F( [], tupleSubscript)
BIND_L( iteratorValue, { f.returnValue(f.getObject<QTuple>(0) .at(f.getNum(1))); })
BIND_F(iterate, tupleIterate)
BIND_L(length, { f.returnValue(static_cast<double>(f.getObject<QTuple>(0).length)); })
BIND_F(toString, tupleToString)
BIND_F(hashCode, tupleHashCode)
BIND_F(*, tupleTimes)
BIND_F(==, tupleEquals)
BIND_F(compare, tupleCompare)
;

tupleMetaClass
->copyParentMethods()
BIND_F( (), tupleInstantiate)
BIND_F(of, tupleFromSequence)
;
}