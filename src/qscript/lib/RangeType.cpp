#include "SwanLib.hpp"
using namespace std;

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

static void rangeSubscript (QFiber& f) {
QRange& r = f.getObject<QRange>(0);
double n = f.getNum(1);
double re = r.start + n * r.step;
f.returnValue( (r.end-re)*r.step >= 0? QV(re) : QV());
}

QV rangeMake (QVM& vm, double start, double end, bool inclusive) {
double step = end>=start? 1 : -1;
return new QRange(vm, start, end, step, inclusive);
}

static void rangeToString (QFiber& f) {
QRange& r = f.getObject<QRange>(0);
if (r.step==1 || r.step==-1) f.returnValue(format("%g%s%g", r.start, r.inclusive?"...":"..", r.end));
else f.returnValue(format("Range(%g, %g, %g, %s)", r.start, r.end, r.step, r.inclusive));
}



void QVM::initRangeType () {
rangeClass
->copyParentMethods()
BIND_L(iteratorValue, { f.returnValue(f.at(1)); })
BIND_L(iterate, { f.returnValue(f.getObject<QRange>(0) .iterate(f.at(1))); })
BIND_F(toString, rangeToString)
BIND_F(in, rangeIn)
BIND_F([], rangeSubscript)
BIND_L(start, { f.returnValue(f.getObject<QRange>(0).start); })
BIND_L(end, { f.returnValue(f.getObject<QRange>(0).end); })
;


rangeMetaClass
->copyParentMethods()
BIND_F( (), rangeInstantiate)
;
}
