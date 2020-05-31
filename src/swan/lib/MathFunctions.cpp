#include "../vm/Tuple.hpp"
#include "SwanLib.hpp"
#include<cmath>
#include<numeric>
using namespace std;

static double signum (double d) {
return copysign(d==0?0:1,d);
}

static double phi (double a, double b) {
double c = sqrt(2);
return (erf(b/c) - erf(a/c))/2;
}

template<double(*F)(double)> static void numMathFunc (QFiber& f) {
f.returnValue(F(f.getNum(0)));
}

template<double(*F)(double,double*)> static void numMathFunc2P (QFiber& f) {
QV re[2] = { 0, 0 };
re[0].d = F(f.getNum(0), &re[1].d);
f.returnValue(QTuple::create(f.vm, 2, re));
}

template<double(*F)(double,int*)> static void numMathFunc2P (QFiber& f) {
QV re[2] = { 0, 0 };
int a = 0;
re[0].d = F(f.getNum(0), &a);
re[1].d = static_cast<double>(a);
f.returnValue(QTuple::create(f.vm, 2, re));
}

template<char C> void genericComp (QFiber& f) {
int nArgs = f.getArgCount();
if (nArgs<2) return;
int symbol = f.vm.findMethodSymbol(string(1,C));
QV val;
f.pushCopy(0);
f.pushCopy(1);
f.pushCppCallFrame();
f.callSymbol(symbol, 2);
f.popCppCallFrame();
val = f.at(f.getBool(-1)? 0 : 1);
f.pop();
for (int i=2; i<nArgs; i++) {
f.push(val);
f.pushCopy(i);
f.pushCppCallFrame();
f.callSymbol(symbol, 2);
f.popCppCallFrame();
if (!f.getBool(-1)) val = f.at(i);
f.pop();
}
f.returnValue(val);
}

template<double(*func)(double)> void numRoundingFunc (QFiber& f) {
double value  = f.getNum(0);
double power = pow(10, f.getOptionalNum(1, 0));
f.returnValue( func(value * power) / power );
}

#define F(X) \
static void num_##X (QFiber& f) { \
f.returnValue(X(f.getNum(0), f.getNum(1))); \
}
F(atan2) F(ldexp)
F(nexttoward) F(nextafter)
F(phi)
#undef F

static void numLog (QFiber& f) {
double d = f.getNum(0), base = f.getOptionalNum(1, -1);
f.returnValue(base>0? log(d)/log(base) : log(d));
}

static void numGCD (QFiber& f) {
int nArgs = f.getArgCount();
if (nArgs<2) return;
uint64_t n = gcd(static_cast<uint64_t>(f.getNum(0)), static_cast<uint64_t>(f.getNum(1)) );
for (int i=2;  i<nArgs; i++) n = gcd(n, static_cast<uint64_t>(f.getNum(i)) );
f.returnValue(static_cast<double>(n));
}

static void numLCM (QFiber& f) {
int nArgs = f.getArgCount();
if (nArgs<2) return;
uint64_t n = lcm(static_cast<uint64_t>(f.getNum(0)), static_cast<uint64_t>(f.getNum(1)) );
for (int i=2; i<nArgs; i++) n = lcm(n, static_cast<uint64_t>(f.getNum(i)) );
f.returnValue(static_cast<double>(n));
}

void QVM::initMathFunctions () {
#define F(X) bindGlobal(#X, numMathFunc<X>, "NN");
F(abs) F(signum)
F(sin) F(cos) F(tan) F(asin) F(acos) F(atan)
F(sinh) F(cosh) F(tanh) F(asinh) F(acosh) F(atanh)
F(exp) F(exp2) F(expm1) 
F(sqrt) F(cbrt) 
F(log2) F(log10) F(log1p)
F(erf) F(tgamma) F(lgamma)
#undef F
#define F(X) bindGlobal(#X, numMathFunc2P<X>, "NCT2NN");
F(modf) F(frexp)
#undef F
#define F(X) bindGlobal(#X, numRoundingFunc<X>, "NNN");
F(floor) F(ceil) F(round) F(trunc)
#undef F
#define F(X) bindGlobal(#X, num_##X, "NNN");
F(atan2) F(ldexp)
F(nexttoward) F(nextafter)
F(phi)
#undef F
bindGlobal("log", numLog, "NNN");
bindGlobal("gcd", numGCD, "NN+N");
bindGlobal("lcm", numLCM, "NN+N");
bindGlobal("min", genericComp<'<'>, "**+@0");
bindGlobal("max", genericComp<'>'>, "**+@0");

bindGlobal("NaN", QV(QV_NAN));
bindGlobal("INFINITY", QV(QV_PLUS_INF));
bindGlobal("POSITIVE_INFINITY", QV(QV_PLUS_INF));
bindGlobal("NEGATIVE_INFINITY", QV(QV_MINUS_INF));
bindGlobal("PI", acos(-1));
bindGlobal("\xCF\x80", acos(-1));
}
