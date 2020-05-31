#ifndef NO_REGEX
#include "SwanLib.hpp"
#include "../vm/Regex.hpp"
#include<boost/algorithm/string.hpp>
#include<utf8.h>
using namespace std;

void stringFind (QFiber& f);
void stringSplitWithoutRegex (QFiber& f);
void stringReplaceWithoutRegex (QFiber& f);

static void regexInstantiate (QFiber& f) {
QString &pattern = f.getObject<QString>(1);
auto opt = QRegex::parseOptions(f.getArgCount()>2 && f.isString(2)? f.getObject<QString>(2).begin() : nullptr);
QRegex* regex = f.vm.construct<QRegex>(f.vm, pattern.begin(), pattern.end(), opt.first, opt.second);
f.returnValue(regex);
}

static void regexIteratorNext (QFiber& f) {
QRegexIterator& r = f.getObject<QRegexIterator>(0);
if (r.it==r.end) f.returnValue(QV::UNDEFINED);
else {
QRegexMatchResult* mr = f.vm.construct<QRegexMatchResult>(f.vm);
mr->match = *r.it++;
f.returnValue(mr);
}}

static void regexTokenIteratorNext (QFiber& f) {
QRegexTokenIterator& r = f.getObject<QRegexTokenIterator>(0);
if (r.it==r.end) f.returnValue(QV::UNDEFINED);
else {
f.returnValue(*r.it++);
}}

static void regexMatchResultStart (QFiber& f) {
QRegexMatchResult& m = f.getObject<QRegexMatchResult>(0);
#ifdef USE_BOOST_REGEX
if (f.isString(1)) f.returnValue(static_cast<double>(m.match.position(f.getObject<QString>(1).begin())));
else 
#endif
f.returnValue(static_cast<double>(m.match.position(f.getOptionalNum(1, 0))));
}

static void regexMatchResultLength (QFiber& f) {
QRegexMatchResult& m = f.getObject<QRegexMatchResult>(0);
#ifdef USE_BOOST_REGEX
if (f.isString(1)) f.returnValue(static_cast<double>(m.match.length(f.getObject<QString>(1).begin())));
else 
#endif
f.returnValue(static_cast<double>(m.match.length(f.getOptionalNum(1, 0))));
}

static void regexMatchResultEnd (QFiber& f) {
QRegexMatchResult& m = f.getObject<QRegexMatchResult>(0);
#ifdef USE_BOOST_REGEX
if (f.isString(1)) {
auto i = f.getObject<QString>(1).begin();
f.returnValue(static_cast<double>(m.match.position(i) + m.match.length(i)));
}
else 
#endif
{
int i = f.getOptionalNum(1, 0);
f.returnValue(static_cast<double>(m.match.position(i) + m.match.length(i)));
}}

static void regexMatchResultSubscript (QFiber& f) {
QRegexMatchResult& m = f.getObject<QRegexMatchResult>(0);
auto& sub =  
#ifdef USE_BOOST_REGEX
f.isString(1)?
m.match[ f.getObject<QString>(1).begin() ]:
#endif
m.match[ f.getOptionalNum(1, 0) ];
f.returnValue(QV(QString::create(f.vm, sub.first, sub.second), QV_TAG_STRING));
}

static void regexTest (QFiber& f) {
QRegex& r = f.getObject<QRegex>(0);
QString& s = f.getObject<QString>(1);
cmatch unused;
f.returnValue(regex_match(const_cast<const char*>(s.begin()), const_cast<const char*>(s.end()), unused, r.regex, r.matchOptions | REGEX_TEST_MATCH_FLAGS));
}

static void regexLength (QFiber& f) {
auto& r = f.getObject<QRegex>(0);
f.returnValue(static_cast<double>(r.regex.mark_count())); 
}

void stringSearch (QFiber& f) {
if (f.isString(1)) { stringFind(f); return; }
QString& s = f.getObject<QString>(0);
QRegex& r = f.getObject<QRegex>(1);
int re=-1, start = f.getOptionalNum(2, 0);
bool full = f.getOptionalBool(2, false) || f.getOptionalBool(3, false);
auto options = full? regex_constants::match_default : REGEX_SEARCH_NOT_FULL_DEFAULT_OPTIONS;
cmatch match;
if (start<0) start+=s.length;
if (start>0) options |= regex_constants::match_prev_avail;
if (regex_search(const_cast<const char*>(s.begin()+start), const_cast<const char*>(s.end()), match, r.regex, r.matchOptions | options)) re = match.position()+start;
if (full && re<0) f.returnValue(QV::UNDEFINED);
else if (full) f.returnValue(f.vm.construct<QRegexMatchResult>(f.vm, match));
else f.returnValue(static_cast<double>(re));
}

void stringFindAll (QFiber& f) {
QString& s = f.getObject<QString>(0);
QRegex& r = f.getObject<QRegex>(1);
int group = f.getOptionalNum(2, -2);
bool full = f.getOptionalBool(2, false);
if (full) f.returnValue(f.vm.construct<QRegexIterator>(f.vm, s, r, r.matchOptions));
else {
int groupCount = r.regex.mark_count();
if (group<-1 || group>groupCount) group = groupCount==0?0:1;
f.returnValue(f.vm.construct<QRegexTokenIterator>(f.vm, s, r, r.matchOptions, group));
}}

void stringSplitWithRegex (QFiber& f) {
if (f.isString(1)) { stringSplitWithoutRegex(f); return; }
QString& s = f.getObject<QString>(0);
QRegex& r = f.getObject<QRegex>(1);
f.returnValue(f.vm.construct<QRegexTokenIterator>(f.vm, s, r, r.matchOptions, -1));
}

void stringReplaceWithRegex (QFiber& f) {
if (f.isString(1)) { stringReplaceWithoutRegex(f); return; }
QString& s = f.getObject<QString>(0);
QRegex& r = f.getObject<QRegex>(1);
string re;
auto out = back_inserter(re);
if (f.isString(2)) {
QString& fmt = f.getObject<QString>(2);
regex_replace(out, s.begin(), s.end(), r.regex, fmt.begin(), r.matchOptions);
}
else {
QV& fmt = f.at(2);
#ifdef USE_BOOST_REGEX
regex_replace(out, const_cast<const char*>(s.begin()), const_cast<const char*>(s.end()), r.regex, [&](auto& m, auto& out){
#else
auto last = s.begin();
for (regex_iterator<const char*> it(s.begin(), s.end(), r.regex, r.matchOptions), end; it!=end; ++it) {
auto& m = *it;
std::copy(last, s.begin()+m.position(0), out);
last = s.begin()+m.position(0)+m.length(0);
#endif
f.push(fmt);
f.push(f.vm.construct<QRegexMatchResult>(f.vm, m));
f.call(1);
QString* repl = f.ensureString(-1);
std::copy(repl->begin(), repl->end(), out);
f.pop();
#ifdef USE_BOOST_REGEX
return out;
}, r.matchOptions);
#else
}
std::copy(last, s.end(), out);
#endif
}
f.returnValue(re);
}

void QVM::initRegexTypes () {
regexClass
->copyParentMethods()
->bind("test", regexTest, "OSB")
->bind("length", regexLength, "ON")
->assoc<QRegex>();

regexMatchResultClass
->copyParentMethods()
->bind("[]", regexMatchResultSubscript, "OOS")
->bind("start", regexMatchResultStart, "OON")
->bind("end", regexMatchResultEnd, "OON")
->bind("length", regexMatchResultLength, "ON")
->assoc<QRegexMatchResult>();

regexIteratorClass
->copyParentMethods()
->bind("iterator", doNothing, "O@0")
->bind("next", regexIteratorNext, "OO")
->assoc<QRegexIterator>();

regexTokenIteratorClass
->copyParentMethods()
->bind("iterator", doNothing, "O@0")
->bind("next", regexTokenIteratorNext, "OO")
->assoc<QRegexTokenIterator>();

regexClass ->type
->copyParentMethods()
->bind("()", regexInstantiate, "OSS?QRegex;")
->assoc<QClass>();
}
#endif

