#include "SwanLib.hpp"
#include<cmath>
#include<numeric>
using namespace std;

template<double(*F)(double)> static void numMathFunc (QFiber& f) {
f.returnValue(F(f.getNum(0)));
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
#define F(X) \
numClass BIND_F(X, numMathFunc<X>); \
bindGlobal(#X, numMathFunc<X>);
F(abs)
F(sin) F(cos) F(tan) F(asin) F(acos) F(atan)
F(sinh) F(cosh) F(tanh) F(asinh) F(acosh) F(atanh)
F(exp) F(sqrt) F(cbrt) F(log2) F(log10)
#undef F
#define F(X) \
numClass BIND_F(X, numRoundingFunc<X>); \
bindGlobal(#X, numRoundingFunc<X>);
F(floor) F(ceil) F(round) F(trunc)
#undef F
bindGlobal("log", numLog);
bindGlobal("gcd", numGCD);
bindGlobal("lcm", numLCM);
bindGlobal("min", genericComp<'<'>);
bindGlobal("max", genericComp<'>'>);

numClass
BIND_F(log, numLog)
BIND_L(frac, { double unused; f.returnValue(modf(f.getNum(0), &unused)); })
BIND_L(int, { double re; modf(f.getNum(0), &re); f.returnValue(re); })
BIND_L(sign, { double d=f.getNum(0); f.returnValue(copysign(d==0?0:1,d)); })
;

bindGlobal("NaN", QV(QV_NAN));
bindGlobal("INFINITY", QV(QV_PLUS_INF));
bindGlobal("POSITIVE_INFINITY", QV(QV_PLUS_INF));
bindGlobal("NEGATIVE_INFINITY", QV(QV_MINUS_INF));
bindGlobal("PI", acos(-1));
bindGlobal("\xCF\x80", acos(-1));
}
