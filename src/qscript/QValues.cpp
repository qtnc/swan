#include "QValue.hpp"
#include "QValueExt.hpp"
#include<vector>
using namespace std;

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
QObject(type0), vm(vm0), parent(parent0), nFields(nf), name(name0), methods(parent0? parent0->methods : vector<QV>())
{}

QClass* QClass::copyParentMethods () {
methods = parent->methods;
return this;
}

QClass* QClass::mergeMixinMethods (QClass* cls) {
for (int i=methods.size(); i<cls->methods.size(); i++) methods.push_back(QV());
for (int i=0; i<cls->methods.size(); i++) if (!cls->methods[i].isNull()) methods[i] = cls->methods[i];
return this;
}

QClass* QClass::bind (const std::string& methodName, QNativeFunction func) {
int symbol = vm.findMethodSymbol(methodName);
return bind(symbol, QV(func));
}

QClass* QClass::bind (int symbol, const QV& val) {
while (methods.size()<=symbol) methods.push_back(QV());
methods[symbol] = val;
return this;
}

QObject* QClass::instantiate () {
return QInstance::create(this, nFields);
}

QObject* QForeignClass::instantiate () {
return QForeignInstance::create(this, nFields);
}

QString::QString (QVM& vm, uint32_t len): 
QSequence(vm.stringClass), length(len) {}

QString* QString::create (QVM& vm, const std::string& str) {
return QString::create(vm, str.data(), str.length());
}

QString* QString::create (QVM& vm, const char* str, int len) {
if (len<0) len = strlen(str);
QString* s = newVLS<QString, char>(len+1, vm, len);
if (len>0) memcpy(s->data, str, len+1);
s->data[len] = 0;
return s;
}

QString* QString::create (QString* s) { 
return create(s->type->vm, s->data, s->length); 
}

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



QTuple* QTuple::create (QVM& vm, uint32_t length, const QV* data) {
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

std::pair<boost::regex_constants::syntax_option_type, boost::regex_constants::match_flag_type>  QRegex::parseOptions (const char* opt) {
boost::regex_constants::syntax_option_type options = boost::regex::perl;
boost::regex_constants::match_flag_type flags = boost::regex_constants::match_default | boost::regex_constants::format_default;
if (opt) while(*opt) switch(*opt++){
case 'c': options |= boost::regex::collate; break;
case 'E': options |= boost::regex::no_empty_expressions; break;
case 'f': flags |= boost::regex_constants::format_first_only; break;
case 'i': options |= boost::regex::icase; break;
case 'L': 
options &= ~boost::regex::perl; 
options |= boost::regex::literal; 
flags |= boost::regex_constants::format_literal; 
break;
case 'M': options |= boost::regex::no_mod_m; break;
case 'N': 
options |= boost::regex::nosubs; 
flags |= boost::regex_constants::match_nosubs;
break;
case 'o': options |= boost::regex::optimize; break;
case 'S': 
options |= boost::regex::no_mod_s; 
flags |= boost::regex_constants::match_not_dot_newline;
break;
case 's': options |= boost::regex::mod_s; break;
case 'x': options |= boost::regex::mod_x; break;
case 'y': flags |= boost::regex_constants::match_continuous; break;
case 'z': flags |= boost::regex_constants::format_all; break;
}
return std::make_pair(options, flags);
}

QRegex::QRegex (QVM& vm, const char* begin, const char* end, boost::regex_constants::syntax_option_type regexOptions, boost::regex_constants::match_flag_type matchOptions0):
QObject(vm.regexClass), regex(begin, end, regexOptions), matchOptions(matchOptions0)
{}

QRegexIterator::QRegexIterator (QVM& vm, QString& s, QRegex& r, boost::regex_constants::match_flag_type options):
QSequence(vm.regexIteratorClass), str(s), regex(r),
it(s.begin(), s.end(), r.regex, options | r.matchOptions)
{}

QRegexTokenIterator::QRegexTokenIterator (QVM& vm, QString& s, QRegex& r, boost::regex_constants::match_flag_type options, int g):
QSequence(vm.regexTokenIteratorClass), str(s), regex(r), 
it(s.begin(), s.end(), r.regex, g, options | r.matchOptions)
{}


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

