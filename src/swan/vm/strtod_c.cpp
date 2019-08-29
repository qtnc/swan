#include<cstdlib>
#include<cstring>
#include<cmath>
// Locale-independent (but probably quite bad) implementation of strtod

double strtod_c (const char* s, char** endptr = nullptr) {
char* e = const_cast<char*>(s);
long long frac=0, num = strtoll(s, &e, 10);
int exponent=0, fraclen=0;
if (s!=e) {
if (*e=='.' && *++e>='0' && *e<='9') {
frac = strtoull(s=e, &e, 10);
fraclen=e-s;
}
if (*e=='e' || *e=='E') {
exponent = strtol(++e, &e, 10);
}
}
if (endptr) *endptr=e;
double value = num;
if (frac) value += (num<0?-1:1) * frac/pow(10, fraclen);
if (exponent) value *= pow(10, exponent);
return value;
}
