#ifndef ____Q_BASE_H_1___
#define ____Q_BASE_H_1___
#include "../include/QScript.hpp"
#include "../include/cpprintf.hpp"
#include<vector>
#include<unordered_map>
#include<string>
#include<cstring>
#include<memory>
#include<functional>
#include<cstdint>
#include<exception>
#include<functional>
#include<sstream>

//#define _GLIBCXX_DEBUG

#define QV_NAN 0x7FF8000000000000ULL
#define QV_PLUS_INF 0x7FF0000000000000ULL
#define QV_MINUS_INF 0xFFF0000000000000ULL
#define QV_NEG_NAN (QV_NAN | 0x8000000000000000ULL)

#define QV_TRUE 0x7FF8000000000001ULL
#define QV_FALSE 0x7FF8000000000002ULL
#define QV_NULL 0x7FF8000000000003ULL
#define QV_VARARG_MARK 0x7FF8000000000004ULL

#define QV_TAG_GENERIC_SYMBOL_FUNCTION 0x7FF9000000000000ULL
#define QV_TAG_UNUSED_1 0x7FFA000000000000ULL
#define QV_TAG_NATIVE_FUNCTION 0x7FFB000000000000ULL
#define QV_TAG_OPEN_UPVALUE 0x7FFC000000000000ULL
#define QV_TAG_UNUSED_3 0x7FFD000000000000ULL
#define QV_TAG_STD_FUNCTION  0x7FFE000000000000ULL
#define QV_TAGMASK 0xFFFF000000000000ULL

#define QV_TAG_STRING 0xFFF9000000000000ULL
#define QV_TAG_NORMAL_FUNCTION 0xFFFA000000000000ULL
#define QV_TAG_BOUND_FUNCTION 0xFFFB000000000000ULL
#define QV_TAG_CLOSURE 0xFFFC000000000000ULL
#define QV_TAG_DATA 0xFFFD000000000000ULL
#define QV_TAG_UNUSED_2 0xFFFE000000000000ULL
#define QV_TAG_FIBER  0xFFFF000000000000ULL

typedef uint16_t uint_jump_offset_t;
typedef uint16_t uint_method_symbol_t;
typedef uint16_t uint_global_symbol_t;
typedef uint16_t uint_constant_index_t;
typedef uint8_t uint_upvalue_index_t;
typedef uint8_t uint_local_index_t;
typedef uint8_t uint_field_index_t;

template<class T> inline T& nullref () { return *reinterpret_cast<T*>(0); }

template<class T, class U, class... A> inline T* newVLS (int nU, A&&... args) {
char* ptr = new char[sizeof(T) + nU*sizeof(U)];
U* uPtr = reinterpret_cast<U*>(ptr + sizeof(T));
new(ptr) T( std::forward<A>(args)... );
new(uPtr) U[nU];
return reinterpret_cast<T*>(ptr);
}

template<class I, class F> I find_last_if (I begin, const I& end, const F& eq) {
I last = end;
for (; begin!=end; ++begin) {
if (eq(*begin)) last = begin;
}
return last;
}

template<class I, class F> std::pair<I,I> find_consecutive (const I& begin, const I& end, const F& pred) {
auto first = std::find_if(begin, end, pred);
if (first==end) return std::make_pair(end, end);
auto last = std::find_if_not(first, end, pred);
return std::make_pair(first, last);
}

template<class T, class C> static inline void insert_n (C& container, int n, const T& val) {
if (n>0) std::fill_n(std::back_inserter(container), n, val);
}

template<class T, class F = std::function<void(const T*, const T*)>, class Alloc = std::allocator<T>> struct execution_stack {
T *base, *top, *finish;
Alloc allocator;
F callback;
inline execution_stack (const F& cb = nullptr, size_t initialCapacity = 4):  base(nullptr), top(nullptr), finish(nullptr), callback(cb) {
base = allocator.allocate(initialCapacity);
top = base;
finish = base+initialCapacity;
}
inline ~execution_stack () { allocator.deallocate(base, (finish-base) * sizeof(T)); }
void reserve (size_t newCapacity) {
if (newCapacity<=finish-base) return;
T* newBase = allocator.allocate(newCapacity);
memcpy(newBase, base, (top-base) * sizeof(T));
callback(base, newBase);
allocator.deallocate(base, (finish-base) * sizeof(T));
top = newBase + (top-base);
base = newBase;
finish = base + newCapacity;
}
inline void resize (size_t newSize) {
reserve(newSize);
top = base + newSize;
}
inline void push_back (const T& x) {
if (top==finish)  reserve(2 * (finish-base));
*top++ = x;
}
inline T& pop_back () {
return *top--;
}
template<class I> void insert (T* pos, I begin, const I& end) {
size_t index = pos-base, len = std::distance(begin, end);
reserve( (top-base) + len);
pos = base+index;
memmove(pos+len, pos, (top-pos) * sizeof(T));
while(begin!=end) *pos++ = *begin++;
top += len;
}
inline void erase (T* begin, T* end) { 
memmove(begin, end, (top-end)*sizeof(T));
top -= (end-begin);
}
inline void insert (T* pos, const T& val) { insert(pos, &val, &val+1); }
inline void erase (T* pos) { erase(pos, pos+1); }
inline size_t size () const { return top-base; }
inline bool empty () { return size()==0; }
inline T& at (int i) { return *(base+i); }
inline T& operator[] (int i) { return *(base+i); }
inline const T& at (int i) const { return *(base+i); }
inline const T& operator[] (int i) const { return *(base+i); }
inline T* begin () { return base; }
inline const T* begin () const { return base; }
inline T* end () { return top; }
inline const T* end () const { return top; }
inline T& back () { return *(top -1); }
inline const T& back () const { return *(top -1); }
};

enum FiberState {
INITIAL,
RUNNING,
YIELDED,
FINISHED,
FAILED
};

struct QFiber;
typedef void(*QNativeFunction)(QFiber&);

struct QVM;
struct QClass;

struct QObject {
QClass* type;
QObject* next;
QObject (QClass* tp);
virtual ~QObject() = default;
virtual bool gcVisit ();
};

union QV {
uint64_t i;
double d;

std::string print () const;

inline QV(): i(QV_NULL) {}
inline explicit QV(uint64_t x): i(x) {}
inline QV(double x): d(x) {}
inline QV(int x): d(x) {}
inline QV(uint32_t  x): d(x) {}
inline QV(bool b): i(b? QV_TRUE : QV_FALSE) {}
inline QV (void* x, uint64_t tag = QV_TAG_DATA): i( reinterpret_cast<uintptr_t>(x) | tag) {}
inline QV (QObject* x, uint64_t tag = QV_TAG_DATA): i( reinterpret_cast<uintptr_t>(x) | tag) {}
inline QV (QNativeFunction f): i(reinterpret_cast<uintptr_t>(f) | QV_TAG_NATIVE_FUNCTION) {}
inline QV (struct QString* s): QV(s, QV_TAG_STRING) {}
QV (QVM& vm, const std::string& s);
QV (QVM& vm, const char* s, int length=-1);

inline bool hasTag (uint64_t tag) const { return (i&QV_TAGMASK)==tag; }
inline bool isNull () const { return i==QV_NULL; }
inline bool isTrue () const { return i==QV_TRUE; }
inline bool isFalse () const { return i==QV_FALSE; }
inline bool isFalsy () const { return i==QV_FALSE || i==QV_NULL; }
inline bool isBool () const { return i==QV_TRUE || i==QV_FALSE; }
inline bool isObject () const { return (i&QV_NEG_NAN)==QV_NEG_NAN && i!=QV_NEG_NAN; }
inline bool isNum () const { return (i&QV_NAN)!=QV_NAN || i==QV_NAN || i==QV_NEG_NAN; }
inline bool isInteger () const { return isNum() && static_cast<int>(d)==d; }
inline bool isInt8 () const { return isInteger() && d>=-127 && d<=127; }
inline bool isClosure () const { return hasTag(QV_TAG_CLOSURE); }
inline bool isNormalFunction () const { return hasTag(QV_TAG_NORMAL_FUNCTION); }
inline bool isNativeFunction () const { return hasTag(QV_TAG_NATIVE_FUNCTION); }
inline bool isBoundFunction () const { return hasTag(QV_TAG_BOUND_FUNCTION); }
inline bool isGenericSymbolFunction () { return hasTag(QV_TAG_GENERIC_SYMBOL_FUNCTION); }
inline bool isFiber () const { return hasTag(QV_TAG_FIBER); }
inline bool isString () const { return hasTag(QV_TAG_STRING); }
inline bool isOpenUpvalue () const { return hasTag(QV_TAG_OPEN_UPVALUE); }

inline bool asBool () const { return i==QV_TRUE; }
inline double asNum () const { return d; }
template<class T = int> inline T asInt () const { return static_cast<T>(i&~QV_TAGMASK); }
template<class T> inline T* asObject () const { return static_cast<T*>(reinterpret_cast<QObject*>(static_cast<uintptr_t>( i&~QV_TAGMASK ))); }
template<class T> inline T* asPointer () const { return reinterpret_cast<T*>(static_cast<uintptr_t>( i&~QV_TAGMASK )); }
inline QNativeFunction asNativeFunction () const { return reinterpret_cast<QNativeFunction>(asPointer<void>()); }
inline bool isInstanceOf (QClass* tp) const { return isObject() && asObject<QObject>()->type==tp; }
std::string asString () const;
const char* asCString () const;
const QS::Range& asRange () const;

QClass& getClass (QVM& vm);
inline void gcVisit () { if (isObject()) asObject<QObject>()->gcVisit(); }
};

struct QSequence: QObject {
QSequence (QClass* c): QObject(c) {}
virtual void insertIntoVector (struct QFiber& f, std::vector<QV>& list, int start);
virtual void insertIntoSet (struct QFiber& f, struct QSet&);
virtual void join (struct QFiber& f, const std::string& delim, std::string& out);
};

struct QString: QSequence {
uint32_t length;
char data[];
static QString* create (QVM& vm, const std::string& str);
static QString* create (QVM& vm, const char* str, int length = -1);
static inline QString* create (QVM& vm, const char* start, const char* end) { return create(vm, start, end-start); }
static QString* create (QString*);
QString (QVM& vm, uint32_t len);
inline std::string asString () { return std::string(data, length); }
inline char* begin () { return data; }
inline char* end () { return data+length; }
virtual ~QString () = default;
};

struct QInstance: QSequence {
QV fields[];
QInstance (QClass* type): QSequence(type) {}
static inline QInstance* create (QClass* type, int nFields) { return newVLS<QInstance, QV>(nFields, type); }
virtual ~QInstance () = default;
virtual bool gcVisit () final override;
};

struct QForeignInstance: QSequence  {
char userData[];
QForeignInstance (QClass* type): QSequence(type) {}
static inline QForeignInstance* create (QClass* type, int nBytes) { return newVLS<QForeignInstance, char>(nBytes, type); }
virtual ~QForeignInstance ();
};

struct QClass: QObject {
QVM& vm;
QClass* parent;
std::string name;
std::vector<QV> methods;
int nFields;
QV staticFields[];
QClass (QVM& vm, QClass* type, QClass* parent, const std::string& name, int nFields=0);
QClass* copyParentMethods ();
QClass* mergeMixinMethods (QClass* mixin);
QClass* bind (const std::string& methodName, QNativeFunction func);
QClass* bind (int symbol, const QV& value);
inline bool isSubclassOf (QClass* cls) { return this==cls || (parent && parent->isSubclassOf(cls)); }
inline QV findMethod (int symbol) {
QV re = symbol>=methods.size()? QV() : methods[symbol];
if (re.isNull() && parent) return parent->findMethod(symbol);
else return re;
}
static QClass* create (QVM& vm, QClass* type, QClass* parent, const std::string& name, int nStaticFields=0, int nFields=0) { return newVLS<QClass, QV>(nStaticFields, vm, type, parent, name, nFields); }
virtual QObject* instantiate ();
virtual ~QClass () = default;
virtual bool gcVisit () final override;
};

struct QForeignClass: QClass {
typedef void(*DestructorFn)(void*);
DestructorFn destructor;
size_t id;
QForeignClass (QVM& vm, QClass* type, QClass* parent, const std::string& name, int nUserBytes=0, DestructorFn=nullptr);
virtual QObject* instantiate () override;
virtual ~QForeignClass () = default;
};

struct QFunction: QObject {
struct Upvalue {
int slot;
bool upperUpvalue;
};
std::string bytecode, name, file;
std::vector<Upvalue> upvalues;
std::vector<QV> constants;
uint8_t nArgs;
bool vararg;

QFunction (QVM& vm);
virtual bool gcVisit () final override;
virtual ~QFunction () = default;
};

struct QClosure: QObject {
QFunction& func;
struct Upvalue* upvalues[];
QClosure (QVM& vm, QFunction& f);
virtual ~QClosure () = default;
virtual bool gcVisit () final override;
};

struct QCallFrame {
QClosure* closure;
const char* bcp;
uint32_t stackBase;
template<class T> inline T read () { return *reinterpret_cast<const T*&>(bcp)++; }
inline bool isCppCallFrame () { return !closure || !bcp; }
};

struct QCatchPoint {
uint32_t stackSize, callFrame, catchBlock, finallyBlock;
};

struct QFiber: QS::Fiber, QSequence  {
typedef execution_stack<QV> Stack;
static thread_local QFiber* curFiber;
Stack stack;
std::vector<QCallFrame> callFrames;
std::vector<QCatchPoint> catchPoints;
std::vector<Upvalue*> openUpvalues;
QVM& vm;
FiberState state;

inline void returnValue (QV value) { stack.at(callFrames.back().stackBase) = value; }
inline void returnValue (const std::string& s) { returnValue(QV(vm,s)); }

inline QV& at (int i) {
return i>=0? stack.at(callFrames.back().stackBase+i) : *(stack.end() +i);
}

virtual inline int getArgCount () final override { return stack.size() - callFrames.back().stackBase; }

virtual inline bool isNum (int i) final override { return at(i).isNum(); }
virtual inline bool isBool  (int i) final override { return at(i).isBool(); }
virtual inline bool isString (int i) final override { return at(i).isString(); }
virtual inline bool isRange (int i) final override;
virtual inline bool isBuffer (int i) final override;
virtual inline bool isNull (int i) final override { return at(i).isNull(); }

template<class T> inline T& getObject (int i) { return *at(i).asObject<T>(); }
virtual inline double getNum (int i) final override { return at(i).asNum(); }
virtual inline bool getBool (int i) final override { return at(i).asBool(); }
virtual inline std::string getString (int i) final override { return at(i).asString(); }
virtual inline const char* getCString (int i) final override { return at(i).asCString(); }
virtual inline const QS::Range& getRange (int i) final override { return at(i).asRange(); }
virtual const void* getBufferV (int i, int* length = nullptr) final override;
virtual inline void* getUserPointer (int i) final override { return getObject<QForeignInstance>(i).userData; }

virtual inline double getOptionalNum (int i, double defaultValue) { return getArgCount()>i && isNum(i)? getNum(i) : defaultValue; }
virtual inline bool getOptionalBool (int i, bool defaultValue) { return getArgCount()>i && isBool(i)? getBool(i) : defaultValue; }
virtual inline std::string getOptionalString  (int i, const std::string& defaultValue) { return getArgCount()>i && isString(i)? getString(i) : defaultValue; }

virtual inline void setNum (int i, double d) final override { at(i).d = d; }
virtual inline void setBool (int i, bool b) final override { at(i) = QV(b); }
virtual inline void setString  (int i, const std::string& s) final override;
virtual inline void setCString  (int i, const char* s) final override;
virtual inline void setBuffer  (int i, const void* data, int length) final override;
virtual void setRange  (int i, const QS::Range& r) final override;
virtual inline void setNull (int i) final override { at(i) = QV(); }
virtual void* setNewUserPointer (int i, size_t id) final override;
inline void setObject (int i, QObject* obj) { at(i)=QV(obj); }
inline void setObject (int i, QObject& obj) { setObject(i, &obj); }

virtual inline void pushNum (double d) final override { stack.push_back(d); }
virtual inline void pushBool  (bool b) final override { stack.push_back(b); }
virtual inline void pushString (const std::string& s) final override;
virtual inline void pushCString (const char* s) final override;
virtual inline void pushBuffer  (const void* data, int length) final override;
virtual void pushRange (const QS::Range& r) final override;
virtual inline void pushNull  () final override { stack.push_back(QV()); }
virtual inline void pushNativeFunction (QS::NativeFunction f) final override { stack.push_back(QV(reinterpret_cast<void*>(f), QV_TAG_NATIVE_FUNCTION)); }
virtual void pushNewForeignClass (const std::string& name, size_t id, int nUserBytes, int nParents=0) final override;
virtual void* pushNewUserPointer (size_t id) final override;
virtual inline void pushCopy (int i = -1) final override { stack.push_back(at(i)); }
virtual inline void pop () final override { stack.pop_back(); }
inline QV& top () { return at(-1); }
inline void push (const QV& x) { stack.push_back(x); }
inline void pushCppCallFrame () { callFrames.push_back({ nullptr, nullptr, stack.size() }); }
inline void popCppCallFrame () { callFrames.pop_back(); }

inline QString* ensureString (QV& val);
inline QString* ensureString (int i) { return ensureString(at(i)); }

virtual void loadFile (const std::string& filename) final override;
virtual void loadString  (const std::string& source, const std::string& name="") final override;

virtual void storeGlobal (const std::string& name) final override;
virtual void loadGlobal (const std::string& name) final override;
virtual void storeMethod (const std::string& name) final override;
virtual void storeStaticMethod (const std::string& name) final override;
virtual void storeDestructor ( void(*destr)(void*) ) final override;
virtual void call (int nArgs) final override;
virtual void callMethod (const std::string& name, int nArgs) final override;

QFiber (QVM& vm);
QFiber (QVM& vm0, QClosure& closure): QFiber(vm0) { callFrames.push_back({ &closure,  closure.func.bytecode.data() , 0 }); }
void adjustArguments (int& nArgs, int nClosureArgs, bool vararg);
void storeMethod (int symbol);
void storeStaticMethod (int symbol);
void pushNewClass (int nParents, int nStaticFields, int nFields);
void callSymbol (int symbol, int nArgs);
void callSuperSymbol (int symbol, int nArgs);
bool callMethod (QV& callable, int nArgs);
void callCallable (int nArgs);
void callClosure (QClosure& closure, int nArgs);
void callFiber (QFiber& f, int nArgs);

Upvalue* captureUpvalue (int slot);
void closeUpvalues (int startSlot);
void adjustUpvaluePointers (const QV* oldPtr, const QV* newPtr);
void handleException (const std::exception& e);

FiberState run ();
template<class... A> void runtimeError (const char* msg, const A&... args);

virtual ~QFiber () = default;
virtual bool gcVisit () override;
};

struct Upvalue: QObject {
QFiber* fiber;
QV value;
static inline int stackpos (const QFiber& f, int n) { return f.callFrames.back().stackBase+n; }
Upvalue (QFiber& f, int slot);
inline QV& get () {
if (value.isOpenUpvalue()) return *value.asPointer<QV>();
else return value;
}
inline void close () { value = get(); }
virtual bool gcVisit () override;
virtual ~Upvalue() = default;
};

struct BoundFunction: QObject {
QV object, method;
BoundFunction (QVM& vm, const QV& o, const QV& m);
virtual ~BoundFunction () = default;
};

struct QVM: QS::VM  {
static std::unordered_map<std::string, EncodingConversionFn> stringToBufferConverters;
static std::unordered_map<std::string, DecodingConversionFn> bufferToStringConverters;

std::vector<std::string> methodSymbols, globalSymbols;
std::vector<QV> globalVariables;
std::unordered_map<std::string,QV> imports;
std::unordered_map<size_t, QForeignClass*> foreignClassIds;
PathResolverFn pathResolver;
FileLoaderFn fileLoader;
CompilationMessageFn messageReceiver;
QObject* firstGCObject;
QClass *boolClass, *bufferClass, *classClass, *fiberClass, *functionClass, *listClass, *mapClass, *nullClass, *numClass, *objectClass, *rangeClass, *sequenceClass, *setClass, *stringClass, *tupleClass;
QClass *bufferMetaClass, *fiberMetaClass, *functionMetaClass, *listMetaClass, *mapMetaClass, *numMetaClass, *setMetaClass, *stringMetaClass, *rangeMetaClass, *tupleMetaClass;
#ifndef NO_REGEX
QClass *regexClass, *regexMatchResultClass, *regexMetaClass, *regexIteratorClass, *regexTokenIteratorClass;
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
QClass *dictionaryClass, *dictionaryMetaClass, *linkedListClass, *linkedListMetaClass;
#endif
#ifndef NO_RANDOM
QClass *randomClass, *randomMetaClass;
#endif
uint8_t varDeclMode = Option::VAR_STRICT;

QVM ();
virtual ~QVM ();
virtual QFiber* createFiber () final override;

int findMethodSymbol (const std::string& name);
int findGlobalSymbol (const std::string& name, bool createNew);
void bindGlobal (const std::string& name, const QV& value);
QClass* createNewClass (const std::string& name, std::vector<QV>& parents, int nStaticFields, int nFields, bool foreign);

virtual void garbageCollect () final override;

virtual inline void setPathResolver (const PathResolverFn& fn) final override { pathResolver=fn; }
virtual inline void setFileLoader (const FileLoaderFn& fn) final override { fileLoader=fn; }
virtual inline void setCompilationMessageReceiver (const CompilationMessageFn& fn) final override { messageReceiver = fn; }
virtual void setOption (Option opt, int value) final override;
virtual int getOption (Option opt) final override;
};

inline void QFiber::setString  (int i, const std::string& s)  { at(i) = QV(vm,s); }
inline void QFiber::pushString (const std::string& s) { stack.push_back(QV(vm,s)); }
inline void QFiber::setCString  (int i, const char* s)  { at(i) = QV(vm,s); }
inline void QFiber::pushCString (const char* s) { stack.push_back(QV(vm,s)); }
inline QString* QFiber::ensureString (QV& val) {
if (val.isString()) return val.asObject<QString>();
else {
int toStringSymbol = vm.findMethodSymbol("toString");
pushCppCallFrame();
push(val);
callSymbol(toStringSymbol, 1);
QString* re = at(-1).asObject<QString>();
pop();
popCppCallFrame();
return re;
}}

template<class F> static inline void iterateSequence (QFiber& f, const QV& initial, const F& func) {
int iteratorSymbol = f.vm.findMethodSymbol(("iterator"));
int iterateSymbol = f.vm.findMethodSymbol(("iterate"));
int iteratorValueSymbol = f.vm.findMethodSymbol(("iteratorValue"));
f.push(initial);
f.pushCppCallFrame();
f.callSymbol(iteratorSymbol, 1);
f.popCppCallFrame();
QV iterable = f.at(-1), key, value;
f.pop();
while(true){
f.push(iterable);
f.push(key);
f.pushCppCallFrame();
f.callSymbol(iterateSymbol, 2);
f.popCppCallFrame();
key = f.at(-1);
f.pop();
if (key.isNull()) break;
f.push(iterable);
f.push(key);
f.pushCppCallFrame();
f.callSymbol(iteratorValueSymbol, 2);
f.popCppCallFrame();
value = f.at(-1);
f.pop();
func(value);
}}

struct OpCodeInfo {
int stackEffect, nArgs, argFormat;
};

enum QOpCode {
#define OP(name, stackEffect, nArgs, argFormat) OP_##name
#include "QOpCodes.hpp"
#undef OP
};


#endif
