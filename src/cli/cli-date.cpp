	#include "../include/QScript.hpp"
#include "../include/QScriptBinding.hpp"
#include "../include/cpprintf.hpp"
#include<boost/algorithm/string.hpp>
#include<ctime>
#include<cstdlib>
using namespace std;

extern double nativeClock ();
static const int MONTH_LENGTHS[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

template<int tm::*prop, int delta> static int tmGetter (const tm& t) {
return delta + t.*prop;
}

template<int tm::*prop, int delta> static int tmSetter (tm& t, int n) {
(t.*prop) = n - delta;
_mktime64(&t);
return delta + t.*prop;
}

static __time64_t tmGetTime (tm& t) {
return _mktime64(&t);
}

static __time64_t tmSetTime (tm& t, __time64_t time) {
t = *_localtime64(&time);
return time;
}

static int getTimeZoneOffset () {
__time64_t time = 86400;
tm t = *_localtime64(&time);
if (t.tm_mday==2) return t.tm_min + 60*t.tm_hour;
else if (t.tm_mday==1) return t.tm_min + 60*t.tm_hour -86400;
else return 0;
}

static __time64_t tmGetTimeUTC (tm& t) {
return _mktime64(&t) -getTimeZoneOffset();
}

static __time64_t tmSetTimeUTC (tm& t, __time64_t time) {
t = *_gmtime64(&time);
return time;
}

static string tmToString (const tm& t) {
string s = asctime(&t);
boost::trim(s);
return s;
}

static string tmFormat (const tm& t, const char* fmt) {
char buf[256] = {0};
buf[strftime(buf, sizeof(buf), fmt, &t)] = 0;
return buf;
}

static bool isLeapYear (const tm& t) {
return t.tm_year%4==0 && (t.tm_year%100!=0 || t.tm_year%400==100);
}

static int getLengthOfMonth (const tm& t) {
return MONTH_LENGTHS[t.tm_mon] + (t.tm_mon==1 && isLeapYear(t)? 1 : 0);
}

static int getWeek (const tm& t) {
char buf[4] = {0};
strftime(buf, 4, "%W", &t);
return strtoul(buf, nullptr, 10);
}

static void tmConstruct (QS::Fiber& f) {
int nArgs = f.getArgCount();
if (nArgs==1) {
__time64_t t = _time64(nullptr);
tm re = *_localtime64(&t);
f.setUserObject(0, re);
}
else if (nArgs==2 && f.isNum(1)) {
__time64_t t = f.getNum(1);
tm re = *_localtime64(&t);
f.setUserObject(0, re);
}
else {
tm t;
t.tm_year = f.getOptionalNum(1, 100) -1900;
t.tm_mon = f.getOptionalNum(2, 0) -1;
t.tm_mday = f.getOptionalNum(3, 0);
t.tm_hour = f.getOptionalNum(4, 0);
t.tm_min = f.getOptionalNum(5, 0);
t.tm_sec = f.getOptionalNum(6, 0);
t.tm_yday = -1;
t.tm_wday = -1;
t.tm_isdst = -1;
_mktime64(&t);
f.setUserObject(0, t);
}}

void registerDate (QS::Fiber& f) {
f.registerClass<tm>("Date");
f.registerDestructor<tm>();
f.registerStaticMethod("()", tmConstruct);
f.registerProperty("time", FUNCTION(tmGetTime), FUNCTION(tmSetTime));
f.registerProperty("utcTime", FUNCTION(tmGetTimeUTC), FUNCTION(tmSetTimeUTC));
f.registerMethod("toString", FUNCTION(tmToString));
f.registerMethod("format", FUNCTION(tmFormat));
#define F0(C, P, D) FUNCTION(tm##C##etter<&tm::P, D>)
#define F(NAME, PROP, DELTA) f.registerProperty(#NAME, F0(G, PROP, DELTA), F0(S, PROP, DELTA));
F(seconds, tm_sec, 0)
F(minutes, tm_min, 0)
F(hours, tm_hour, 0)
F(day, tm_mday, 0)
F(weekDay, tm_wday, 0)
F(yearDay, tm_yday, 1)
F(month, tm_mon, 1)
F(year, tm_year, 1900)
#undef F0
#undef F
f.registerMethod("week", FUNCTION(getWeek));
f.registerMethod("lengthOfMonth", FUNCTION(getLengthOfMonth));
f.registerMethod("leapYear", FUNCTION(isLeapYear));
f.pop();

f.loadGlobal("System");
f.registerStaticMethod("clock", STATIC_METHOD(nativeClock));
f.pop();
}
