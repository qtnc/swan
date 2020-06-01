#include "SwanLib.hpp"
#include "../../include/cpprintf.hpp"
#include "../vm/Tuple.hpp"
#include<cmath>
#include<cstdlib>
using namespace std;

QV rangeMake (QVM& vm, double start, double end, bool inclusive);
QV stringToNumImpl (QString& s, int base);
void objectHashCode (QFiber& f);

double dintdiv (double a, double b) {
return static_cast<int64_t>(a) / static_cast<int64_t>(b);
}

double dlshift (double a, double b) {
int64_t x = a, y = b;
return y<0? x>>-y : x<<y;
}

double drshift (double a, double b) {
int64_t x = a, y = b;
return y>0? x>>y : x<<-y;
}

#define OP(O,N) \
static void num##N (QFiber& f) { \
f.returnValue(f.getNum(0) O f.getNum(1)); \
}

#define OPF(F,N) \
static void num##N (QFiber& f) { \
f.returnValue(F(f.getNum(0), f.getNum(1))); \
}

#define OPB(O,N) \
static void num##N (QFiber& f) { \
f.returnValue(static_cast<double>(static_cast<int64_t>(f.getNum(0)) O static_cast<int64_t>(f.getNum(1)))); \
}

OP(+, Plus)
OP(-, Minus)
OP(*, Mul)
OP(/, Div)
OPB(|, BinOr)
OPB(&, BinAnd)
OPB(^, BinXor)
OPF(fmod, Mod)
OPF(pow, Pow)
OPF(dintdiv, IntDiv)
OPF(dlshift, LSH)
OPF(drshift, RSH)
OP(<, Lt)
OP(>, Gt)
OP(<=, Lte)
OP(>=, Gte)
OP(==, Eq)
OP(!=, Neq)
#undef OP
#undef OPF
#undef OPB

static void numNeg (QFiber& f) {
f.returnValue(-f.getNum(0));
}

static void numBinNot (QFiber& f) {
f.returnValue(static_cast<double>(~static_cast<int64_t>(f.getNum(0)))); 
}

static void numToRange (QFiber& f) {
f.returnValue(rangeMake(f.vm, f.getNum(0), f.getNum(1), false)); 
}

static void numToRangeClosed (QFiber& f) {
f.returnValue(rangeMake(f.vm, f.getNum(0), f.getNum(1), true)); 
}

static bool numToStringBase (QFiber& f, double val) {
if (isnan(val)) {
f.returnValue(QV(f.vm, "NaN", 3));
return true;
}
else if (isinf(val)) {
if (val<0) f.returnValue(QV(f.vm, "\xE2\x88\x92\xE2\x88\x9E", 6));
else f.returnValue(QV(f.vm, "+\xE2\x88\x9E", 4));
return true;
}
return false;
}

static void numToString (QFiber& f) {
double val = f.getNum(0);
if (!numToStringBase(f, val)) {
int base = f.getOptionalNum(1, 0);
if (base==0) f.returnValue(QV(f.vm, format("%.14g", val) ));
else if (base>=2 && base<=36) {
char buf[32] = {0};
lltoa(static_cast<int64_t>(val), buf, base);
f.returnValue(QV(f.vm, buf));
}
else error<invalid_argument>("Invalid base (%d), base must be between 2 and 36.", base);
}}

static void numToJSON (QFiber& f) {
f.returnValue(format("%.14g", f.getNum(0)));
}

static void numFormat (QFiber& f) {
double value = f.getNum(0);
int decimals = f.getOptionalNum(1, 2);
string decimalSeparator = f.getOptionalString(2, ".");
string groupSeparator = f.getOptionalString(3, "");
int width = f.getOptionalNum(4, 0);
int groupLength = f.getOptionalNum(5, 3);
string s;
if (decimals>=0 && width<=0) s = format("%.*f", decimals, value);
else if (decimals>=0 && width>0) s = format("%0.*$*f", decimals, width, value);
else if (decimals<0) {
int exponent = log10(value);
double mantissa = value/pow(10, exponent);
if (width>0)  s = format("%.*ge%0$*d", -decimals, mantissa, width, exponent);
else s = format("%.*ge%d", -decimals, mantissa, exponent);
}
auto pos = s.find('.');
if (pos!=string::npos) s.replace(pos, 1, decimalSeparator);
else pos = s.size();
while(pos>groupLength) {
pos -= groupLength;
s.insert(pos, groupSeparator);
}
f.returnValue(QV(f.vm, s));
}

static void numInstantiate (QFiber& f) {
QV& val = f.at(1);
if (val.isNum()) f.returnValue(val);
else if (val.isBool()) f.returnValue(val.asBool()? 1 : 0);
else if (val.isString()) {
int base = f.getOptionalNum(2, -1);
f.returnValue(stringToNumImpl(f.getObject<QString>(1), base));
}
else f.returnValue(QV::UNDEFINED);
}



void QVM::initNumberType () {
numClass
->copyParentMethods()
->bind("+", numPlus, "NNN")
->bind("-", numMinus, "NNN")
->bind("*", numMul, "NNN")
->bind("/", numDiv, "NNN")
->bind("==", numEq, "NNB")
->bind("!=", numNeq, "NNB")
->bind("<", numLt, "NNB")
->bind(">", numGt, "NNB")
->bind("<=", numLte, "NNB")
->bind(">=", numGte, "NNB")
->bind("%", numMod, "NNN")
->bind("**", numPow, "NNN")
->bind("\\", numIntDiv, "NNN")
->bind("<<", numLSH, "NNN")
->bind(">>", numRSH, "NNN")
->bind("|", numBinOr, "NNN")
->bind("&", numBinAnd, "NNN")
->bind("^", numBinXor, "NNN")
->bind("~", numBinNot, "NNN")
->bind("unm", numNeg, "NN")
->bind("unp", doNothing, "O@0")
->bind("<=>", numMinus, "NNN")
->bind("toString", numToString, "NS")
->bind("toJSON", numToJSON)
->bind("format", numFormat, "NN?S?S?N?N?S") 
->bind("..", numToRange, "NNR")
->bind("...", numToRangeClosed, "NNR")
;

numClass ->type
->copyParentMethods()
->bind("()", numInstantiate, "ON?N")
->assoc<QClass>();
}
