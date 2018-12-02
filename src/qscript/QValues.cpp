#include "QValue.hpp"
#include "QValueExt.hpp"
#include<vector>
using namespace std;

#define MAX_CACHED_STRING_LENGTH 16

size_t hashBytes (const uint8_t* c, const uint8_t* end) {
size_t re = FNV_OFFSET;
for (; c<end; ++c) re = (re^*c) * FNV_PRIME;
return re;
}

void appendToString (QFiber& f, QV x, string& out) {
if (x.isString()) {
QString& s = *x.asObject<QString>();
out.insert(out.end(), s.begin(), s.end());
}
else {
f.pushCppCallFrame();
f.push(x);
f.callSymbol(f.vm.findMethodSymbol("toString"), 1);
QString& s = *f.at(-1).asObject<QString>();
out.insert(out.end(), s.begin(), s.end());
f.pop();
f.popCppCallFrame();
}}

QV::QV (QVM& vm, const std::string& s): QV(QString::create(vm, s), QV_TAG_STRING) {}
QV::QV (QVM& vm, const char* s, int length): QV(QString::create(vm, s, length), QV_TAG_STRING) {}

std::string QV::asString () const { return asObject<QString>()->asString(); }
const char* QV::asCString () const { return asObject<QString>()->begin(); }
const QS::Range& QV::asRange () const { return *asObject<QRange>(); }
bool QFiber::isRange (int i) { return at(i).isInstanceOf(vm.rangeClass); }

bool QV::isInstanceOf (QClass* tp) const {
if (!isObject()) return false;
QClass* type = asObject<QObject>()->type;
do {
if (type==tp) return true;
} while ((type=type->parent));
return false;
}

export QS::Handle::Handle (): value(QV_NULL) {}
export QS::Handle::Handle (QS::Handle&& h): value(h.value) { h.value=QV_NULL; }
QS::Handle& export QS::Handle::operator= (Handle&& h) { value=h.value; h.value=QV_NULL; return *this; }

QS::Handle QV::asHandle () {
if (isObject()) {
QObject* obj = asObject<QObject>();
LOCK_SCOPE(obj->type->vm.globalMutex)
obj->type->vm.keptHandles.push_back(*this);
}
QS::Handle h;
h.value = i;
return h;
}

export QS::Handle::~Handle () {
QV qv(value);
if (qv.isObject()) {
QObject* obj = qv.asObject<QObject>();
LOCK_SCOPE(obj->type->vm.globalMutex)
auto& keptHandles = obj->type->vm.keptHandles;
auto it = find_if(keptHandles.begin(), keptHandles.end(), [&](const auto& x){ return x.i==qv.i; });
if (it!=keptHandles.end()) keptHandles.erase(it);
}}

#ifndef NO_BUFFER
bool QFiber::isBuffer (int i) { return at(i).isInstanceOf(vm.bufferClass); }

const void* QFiber::getBufferV (int i, int* length) {
QBuffer& b = getObject<QBuffer>(i);
if (length) *length = b.length;
return b.data;
}

void QFiber::pushBuffer  (const void* data, int length) {
push(QBuffer::create(vm, data, length));
}

void QFiber::setBuffer  (int i, const void* data, int length) {
at(i) = QBuffer::create(vm, data, length);
}
#else
bool QFiber::isBuffer (int i) { return false; }
const void* QFiber::getBufferV (int i, int* length) { return nullptr; }
void QFiber::setBuffer  (int i, const void* data, int length) { }
#endif

void QFiber::pushRange (const QS::Range& r) {
push(new QRange(vm, r));
}

void QFiber::setRange (int i, const QS::Range& r) {
at(i) = new QRange(vm, r);
}

Upvalue::Upvalue (QFiber& f, int slot): 
QObject(f.vm.objectClass), fiber(&f), value(QV(static_cast<uint64_t>(QV_TAG_OPEN_UPVALUE | reinterpret_cast<uintptr_t>(&f.stack.at(stackpos(f, slot)))))) 
{}

QObject::QObject (QClass* tp):
type(tp), next(nullptr) {
if (type && &type->vm) {
next = type->vm.firstGCObject;
type->vm.firstGCObject = this;
}}

QForeignInstance::~QForeignInstance () {
QForeignClass* cls = static_cast<QForeignClass*>(type);
if (cls->destructor) cls->destructor(userData);
}

QFunction::QFunction (QVM& vm): QObject(vm.functionClass), nArgs(0), vararg(false)  {}

QClosure::QClosure (QVM& vm, QFunction& f):
QObject(vm.functionClass), func(f) {}

BoundFunction::BoundFunction (QVM& vm, const QV& o, const QV& m):
QObject(vm.functionClass), object(o), method(m) {}

QClass& QV::getClass (QVM& vm) {
if (isNull()) return *vm.nullClass;
else if (isBool()) return *vm.boolClass;
else if (isNum()) return *vm.numClass;
else if (isNativeFunction())  return *vm.functionClass;
else if (isGenericSymbolFunction())  return *vm.functionClass;
else {
QClass* cls = asObject<QObject>()->type;
return cls? *cls : *vm.classClass;
}}

QForeignClass::QForeignClass (QVM& vm0, QClass* type0, QClass* parent0, const std::string& name0, int nf, DestructorFn destr):
QClass(vm0, type0, parent0, name0, nf), destructor(destr)
{}

QClass::QClass (QVM& vm0, QClass* type0, QClass* parent0, const std::string& name0, int nf):
QObject(type0), vm(vm0), parent(parent0), nFields(nf), name(name0)
{ copyParentMethods(); }

QClass* QClass::copyParentMethods () {
if (parent) methods = parent->methods;
else methods.clear();
return this;
}

QClass* QClass::mergeMixinMethods (QClass* cls) {
insert_n(methods, static_cast<int>(cls->methods.size())-static_cast<int>(methods.size()), QV());
for (int i=0; i<cls->methods.size(); i++) if (!cls->methods[i].isNull()) methods[i] = cls->methods[i];
return this;
}

QClass* QClass::bind (const std::string& methodName, QNativeFunction func) {
int symbol = vm.findMethodSymbol(methodName);
return bind(symbol, QV(func));
}

QClass* QClass::bind (int symbol, const QV& val) {
insert_n(methods, 1+symbol-methods.size(), QV());
methods[symbol] = val;
return this;
}

QObject* QClass::instantiate () {
return QInstance::create(this, nFields);
}

QObject* QForeignClass::instantiate () {
return QForeignInstance::create(this, nFields);
}

QString::QString (QVM& vm, size_t len): 
QSequence(vm.stringClass), length(len) {}

QString* QString::create (QVM& vm, const std::string& str) {
return QString::create(vm, str.data(), str.length());
}

QString* QString::create (QVM& vm, const char* str, int len) {
if (len<0) len = strlen(str);
if (len<=MAX_CACHED_STRING_LENGTH) {
auto it = vm.stringCache.find(make_pair(str, str+len));
if (it!=vm.stringCache.end()) return it->second;
}
QString* s = newVLS<QString, char>(len+1, vm, len);
if (len>0) memcpy(s->data, str, len+1);
s->data[len] = 0;
if (len<=MAX_CACHED_STRING_LENGTH) vm.stringCache.insert(make_pair(make_pair(s->begin(), s->end()), s));
return s;
}

QString* QString::create (QString* s) { 
return create(s->type->vm, s->data, s->length); 
}

QString::~QString () {
if (length<=MAX_CACHED_STRING_LENGTH)  type->vm.stringCache.erase(make_pair(begin(), end()));
}

QTuple* QTuple::create (QVM& vm, size_t length, const QV* data) {
QTuple* tuple = newVLS<QTuple, QV>(length, vm, length);
memcpy(tuple->data, data, length*sizeof(QV));
return tuple;
}

void QSequence::insertIntoVector (QFiber& f, std::vector<QV>& list, int start) {
vector<QV> toInsert;
iterateSequence(f, QV(this), [&](const QV& x){ toInsert.push_back(x); });
if (!toInsert.empty()) list.insert(list.begin()+start, toInsert.begin(), toInsert.end());
}

void QSequence::insertIntoSet (QFiber& f, QSet& set) {
vector<QV> toInsert;
iterateSequence(f, QV(this), [&](const QV& x){ toInsert.push_back(x); });
if (!toInsert.empty()) set.set.insert(toInsert.begin(), toInsert.end());
}

void QSequence::join (QFiber& f, const string& delim, string& re) {
bool notFirst = false;
iterateSequence(f, QV(this), [&](const QV& x){ 
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
 });
}

void QList::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV& x: data) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}}

void QSet::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (const QV& x: set) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}}

void QTuple::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV *x = data, *end=data+length; x<end; x++) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, *x, re);
}}

size_t QVHasher::operator() (const QV& value) const {
QFiber& f = *QFiber::curFiber;
static int hashCodeSymbol = f.vm.findMethodSymbol("hashCode");
f.pushCppCallFrame();
f.push(value);
f.callSymbol(hashCodeSymbol, 1);
size_t re = f.getNum(-1);
f.pop();
f.popCppCallFrame();
return re;
}

bool QVEqualler::operator() (const QV& a, const QV& b) const {
QFiber& f = *QFiber::curFiber;
static int eqeqSymbol = f.vm.findMethodSymbol("==");
f.pushCppCallFrame();
f.push(a);
f.push(b);
f.callSymbol(eqeqSymbol, 2);
bool re = f.getBool(-1);
f.pop();
f.popCppCallFrame();
return re;
}


bool QVLess::operator() (const QV& a, const QV& b) const {
QFiber& f = *QFiber::curFiber;
static int lessSymbol = f.vm.findMethodSymbol("<");
f.pushCppCallFrame();
f.push(a);
f.push(b);
f.callSymbol(lessSymbol, 2);
bool re = f.getBool(-1);
f.pop();
f.popCppCallFrame();
return re;
}

bool QVBinaryPredicate::operator() (const QV& a, const QV& b) const {
QFiber& f = *QFiber::curFiber;
f.pushCppCallFrame();
f.push(func);
f.push(a);
f.push(b);
f.call(2);
bool re = f.getBool(-1);
f.pop();
f.popCppCallFrame();
return re;
}

bool QVUnaryPredicate::operator() (const QV& a) const {
QFiber& f = *QFiber::curFiber;
f.pushCppCallFrame();
f.push(func);
f.push(a);
f.call(1);
bool re = f.getBool(-1);
f.pop();
f.popCppCallFrame();
return re;
}

#ifndef NO_OPTIONAL_COLLECTIONS
void QLinkedList::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV& x: data) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}}
#endif

#ifndef NO_BUFFER
QBuffer::QBuffer (QVM& vm, uint32_t len): 
QSequence(vm.bufferClass), length(len) {}

QBuffer* QBuffer::create (QVM& vm, const void* str, int len) {
QBuffer* s = newVLS<QBuffer, uint8_t>(len+4, vm, len);
if (len>0) memcpy(s->data, str, len);
*reinterpret_cast<uint32_t*>(&s->data[len]) = 0;
return s;
}

QBuffer* QBuffer::create (QBuffer* s) { 
return create(s->type->vm, s->data, s->length); 
}
#endif

#ifndef NO_REGEX
std::pair<regex_constants::syntax_option_type, regex_constants::match_flag_type>  QRegex::parseOptions (const char* opt) {
regex_constants::syntax_option_type options = regex::ECMAScript | regex::optimize;
regex_constants::match_flag_type flags = regex_constants::match_default | regex_constants::format_default;
if (opt) while(*opt) switch(*opt++){
case 'c': options |= regex::collate; break;
case 'f': flags |= regex_constants::format_first_only; break;
case 'i': options |= regex::icase; break;
case 'y': flags |= regex_constants::match_continuous; break;
#ifdef USE_BOOST_REGEX
case 'E': options |= regex::no_empty_expressions; break;
case 'M': options |= regex::no_mod_m; break;
case 'S': 
options |= regex::no_mod_s; 
flags |= regex_constants::match_not_dot_newline;
break;
case 's': options |= regex::mod_s; break;
case 'x': options |= regex::mod_x; break;
case 'z': flags |= regex_constants::format_all; break;
#else
case 'E': flags |= regex_constants::match_not_null; break;
#endif
}
return std::make_pair(options, flags);
}

QRegex::QRegex (QVM& vm, const char* begin, const char* end, regex_constants::syntax_option_type regexOptions, regex_constants::match_flag_type matchOptions0):
QObject(vm.regexClass), regex(begin, end, regexOptions), matchOptions(matchOptions0)
{}

QRegexIterator::QRegexIterator (QVM& vm, QString& s, QRegex& r, regex_constants::match_flag_type options):
QSequence(vm.regexIteratorClass), str(s), regex(r),
it(s.begin(), s.end(), r.regex, options | r.matchOptions)
{}

QRegexTokenIterator::QRegexTokenIterator (QVM& vm, QString& s, QRegex& r, regex_constants::match_flag_type options, int g):
QSequence(vm.regexTokenIteratorClass), str(s), regex(r), 
it(s.begin(), s.end(), r.regex, g, options | r.matchOptions)
{}
#endif

string QS::RuntimeException::getStackTraceAsString () {
ostringstream out;
for (auto& e: stackTrace) {
if (e.line>=0 && !e.function.empty() && !e.file.empty()) println(out, "at %s in %s:%d", e.function, e.file, e.line);
else if (e.line<0 && !e.function.empty() && !e.file.empty()) println(out, "at %s in %s", e.function, e.file);
else if (e.line>=0 && e.function.empty() && !e.file.empty()) println(out, "in %s:%d", e.file, e.line);
else if (e.line<0 && e.function.empty() && !e.file.empty()) println(out, "in %s", e.file);
else if (e.line<0 && !e.function.empty() && e.file.empty()) println(out, "at %s", e.function);
}
return out.str();
}

