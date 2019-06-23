#include "SwanLib.hpp"
#include "../../include/cpprintf.hpp"
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
if (base>=2 && base<=36) {
char buf[32] = {0};
lltoa(static_cast<int64_t>(val), buf, base);
f.returnValue(QV(f.vm, buf));
}
else f.returnValue(QV(f.vm, format("%.14g", val) ));
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
#define OP(O) BIND_L(O, { f.returnValue(f.getNum(0) O f.getNum(1)); })
#define OPF(O,F) BIND_L(O, { f.returnValue(F(f.getNum(0), f.getNum(1))); })
#define OPB(O) BIND_L(O, { f.returnValue(static_cast<double>(static_cast<int64_t>(f.getNum(0)) O static_cast<int64_t>(f.getNum(1)))); })
numClass
->copyParentMethods()
OP(+) OP(-) OP(*) OP(/)
OP(<) OP(>) OP(<=) OP(>=) OP(==) OP(!=)
OPF(%, fmod)
OPF(**, pow)
OPF(\\, dintdiv)
OPF(<<, dlshift) OPF(>>, drshift)
OPB(|) OPB(&) OPB(^)
BIND_L(unm, { f.returnValue(-f.getNum(0)); })
BIND_L(unp, { f.returnValue(+f.getNum(0)); })
BIND_L(~, { f.returnValue(static_cast<double>(~static_cast<int64_t>(f.getNum(0)))); })
BIND_L(compare, { f.returnValue(f.getNum(0)  - f.getNum(1)); })
BIND_F(toString, numToString)
BIND_F(toJSON, numToJSON)
BIND_F(hashCode, objectHashCode)
BIND_F(format, numFormat)
BIND_L(.., { f.returnValue(rangeMake(f.vm, f.getNum(0), f.getNum(1), false)); })
BIND_L(..., { f.returnValue(rangeMake(f.vm, f.getNum(0), f.getNum(1), true)); })
;
#undef OPF
#undef OPB
#undef OP

numClass ->type
->copyParentMethods()
BIND_F( (), numInstantiate)
;
}
