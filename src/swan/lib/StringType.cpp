#include "SwanLib.hpp"
#include "../vm/String.hpp"
#include "../vm/List.hpp"
#include "../vm/NatSort.hpp"
#include "../../include/cpprintf.hpp"
#include<cstdlib>
#include<boost/algorithm/string.hpp>
#include<utf8.h>
using namespace std;

QV stringToNumImpl (QString& s, int base) {
char* end = nullptr;
if (base<0 || base>32) {
double re = strtod(s.begin(), &end);
return end && end==s.end()? re : QV();
} else {
long long re = strtoll(s.begin(), &end, base);
return end && end==s.end()? static_cast<double>(re) : QV();
}}

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
f.returnValue(static_cast<double>(utf8::distance(s.data, s.data+s.length)));
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

void stringFind (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
int start = f.getOptionalNum(2, 0), length = utf8::distance(s->begin(), s->end());
if (start<0) start += length;
auto endPos = s->end(), startPos = s->begin();
utf8::advance(startPos, start, endPos);
auto re = search(startPos, endPos, needle->begin(), needle->end());
if (re==endPos) f.returnValue(-1.0);
else f.returnValue(static_cast<double>(utf8::distance(s->begin(), re)));
}

static void stringRfind (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
int length = utf8::distance(s->begin(), s->end());
int end = f.getOptionalNum(2, length);
if (end<0) end += length;
auto startPos = s->begin(), endPos = startPos;
utf8::advance(endPos, end, s->end());
auto re = find_end(startPos, endPos, needle->begin(), needle->end());
if (re==endPos) f.returnValue(-1.0);
else f.returnValue(static_cast<double>(utf8::distance(s->begin(), re)));
}

static void stringFindFirstOf (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
int start = f.getOptionalNum(2, 0), length = utf8::distance(s->begin(), s->end());
if (start<0) start += length;
auto endPos = s->end(), startPos = s->begin();
utf8::advance(startPos, start, endPos);
auto re = find_first_of(startPos, endPos, needle->begin(), needle->end());
if (re==endPos) f.returnValue(-1.0);
else f.returnValue(static_cast<double>(utf8::distance(s->begin(), re)));
}

static void stringIn (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
auto it = search(s->begin(), s->end(), needle->begin(), needle->end());
f.returnValue(it!=s->end());
}

static void stringStartsWith (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
if (needle->length>s->length) { f.returnValue(QV(false)); return; }
else f.returnValue(equal(needle->begin(), needle->end(), s->begin()));
}

static void stringEndsWith (QFiber& f) {
QString *s = f.at(0).asObject<QString>(), *needle = f.ensureString(1);
if (needle->length>s->length) { f.returnValue(QV(false)); return; }
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

static void stringTrim (QFiber& f) {
QString& s = f.getObject<QString>(0);
string re(s.begin(), s.end());
boost::trim(re);
f.returnValue(QString::create(f.vm, re));
}

static void stringToNum (QFiber& f) {
QString& s = f.getObject<QString>(0);
int base = f.getOptionalNum(1, -1);
f.returnValue(stringToNumImpl(s, base));
}

void stringFormat (QFiber& f) {
QString& fmt = *f.ensureString(0);
ostringstream out;
auto cur = fmt.begin(), end = fmt.end(), last = cur;
while((cur = find_if(last, end, boost::is_any_of("%$"))) < end) {
if (cur>last) out.write(last, cur-last);
if (++cur==end) out << cur[-1];
else if (isDigit(*cur)) {
int i = strtoul(cur, &cur, 10);
if (i<f.getArgCount()) {
QString* s = f.ensureString(i);
out.write(s->data, s->length);
}}
else if (isName(*cur)) {
auto endName = find_if_not(cur, end, isName);
f.push(f.at(1));
f.push(QV(QString::create(f.vm, cur, endName), QV_TAG_STRING));
f.pushCppCallFrame();
f.callSymbol(f.vm.findMethodSymbol("[]"), 2);
f.popCppCallFrame();
if (!f.isNull(-1)) {
QString* s = f.ensureString(-1);
f.pop();
out.write(s->data, s->length);
}
cur = endName;
}
else out << *cur;
last = cur;
}
if (last<end) out.write(last, end-last);
f.returnValue(QString::create(f.vm, out.str()));
}

void stringSplitWithoutRegex (QFiber& f) {
string subject = f.ensureString(0)->asString(), separator = f.ensureString(1)->asString();
vector<string> result;
boost::split(result, subject, boost::is_any_of(separator), boost::token_compress_off);
QList* list = new QList(f.vm);
list->data.reserve(result.size());
for (auto& s: result) list->data.push_back(QV(f.vm, s));
f.returnValue(list);
}

void stringReplaceWithoutRegex (QFiber& f) {
string subject = f.ensureString(0)->asString(), needle = f.ensureString(1)->asString(), replacement = f.ensureString(2)->asString();
boost::replace_all(subject, needle, replacement);
f.returnValue(subject);
}

#ifndef NO_REGEX

void stringSearch (QFiber& f);
void stringFindAll (QFiber& f);
void stringSplitWithRegex (QFiber& f);
void stringReplaceWithRegex (QFiber& f);
#endif


void QVM::initStringType () {
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
BIND_L(compare, { f.returnValue(static_cast<double>(stringCompare(f))); })
OP(==) OP(!=)
OP(<) OP(>) OP(<=) OP(>=)
#undef OP

BIND_F(indexOf, stringFind)
BIND_F(lastIndexOf, stringRfind)
BIND_F(findFirstOf, stringFindFirstOf)
BIND_F(startsWith, stringStartsWith)
BIND_F(endsWith, stringEndsWith)
BIND_F(upper, stringUpper)
BIND_F(lower, stringLower)
BIND_F(toNumber, stringToNum)
BIND_F(unp, stringToNum)
BIND_F(codePointAt, stringCodePointAt)
BIND_F(*, stringTimes)
#ifndef NO_REGEX
BIND_F(search, stringSearch)
BIND_F(split, stringSplitWithRegex)
BIND_F(replace, stringReplaceWithRegex)
BIND_F(findAll, stringFindAll)
#else
BIND_F(split, stringSplitWithoutRegex)
BIND_F(replace, stringReplaceWithoutRegex)
#endif
BIND_F(trim, stringTrim)
BIND_F(format, stringFormat)
;
}
