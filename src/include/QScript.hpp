#ifndef ___QCRIPT_HPP___
#define ___QCRIPT_HPP___

#ifndef __cplusplus
#error C++ is required
#endif

#ifdef __WIN32
#define export __declspec(dllexport)
#else
#define export
#endif

#include<string>
#include<vector>
#include<functional>
#include<exception>
#include<typeinfo>

namespace QS {

struct VM;
struct Fiber;
typedef void(*NativeFunction)(Fiber&);

struct ScriptException: std::exception {
std::string msg;
ScriptException (const std::string& m): msg(m) {}
const char* what () const noexcept final override { return msg.c_str(); }
~ScriptException () = default;
};

struct CompilationException: ScriptException {
bool incomplete;
inline bool isIncomplete () { return incomplete; }
inline CompilationException (bool ic): ScriptException("Compilation failed"), incomplete(ic) {}
~CompilationException () = default;
};

struct CompilationMessage {
enum Kind {
ERROR,
WARNING,
INFO
};
Kind kind;
std::string message, token, file;
int line, column;
};

struct StackTraceElement {
std::string function, file;
int line;
};

struct RuntimeException: ScriptException {
std::vector<StackTraceElement> stackTrace;
std::string type;
int code;
inline RuntimeException (const std::string& cat, const std::string& msg, int c): ScriptException(msg), type(cat), code(c) {}
std::string export getStackTraceAsString ();
};

struct Range {
double start, end, step;
bool inclusive;
Range (double s, double e, double p, bool i=false): start(s), end(e), step(p), inclusive(i) {}
Range (double s, double e, bool i=false): Range(s, e, e>=s?1:-1, i) {}
Range (double e, bool i=false): Range(0, e, i) {}
inline void makeBounds (int length, int& begin, int& finish) const {
begin = start<0? length+start : start;
finish = end<0? length+end : end;
if (inclusive) finish++;
if (begin<0) begin=0; if (finish<0) finish=0;
if (begin>length) begin=length; if (finish>length) finish=length;
if (finish<begin) finish=begin;
}
};

struct Handle {
uint64_t value;
export Handle ();
Handle (const Handle&) = default;
export Handle (Handle&&);
Handle& operator= (const Handle&) = default;
Handle& export operator= (Handle&&);
export ~Handle ();
};

struct Fiber {

Fiber () = default;
Fiber (const Fiber&) = delete;
Fiber& operator= (const Fiber&) = delete;
virtual ~Fiber () = default;

virtual int getArgCount () = 0;
virtual VM& getVM () = 0;

virtual bool isBool (int stackIndex) = 0;
virtual bool isNum (int stackIndex) = 0;
virtual bool isString (int stackIndex) = 0;
virtual bool isRange (int stackIndex) = 0;
virtual bool isBuffer (int stackIndex) = 0;
virtual bool isNull (int stackIndex) = 0;

virtual double getNum (int stackIndex) = 0;
virtual bool getBool (int stackIndex) = 0;
virtual std::string getString (int stackIndex) = 0;
virtual const char* getCString (int stackIndex) = 0;
virtual const Range& getRange (int stackIndex) = 0;
virtual const void* getBufferV (int stackIndex, int* length = nullptr) = 0;
virtual void* getUserPointer (int stackIndex) = 0;
virtual Handle getHandle (int stackIndex) = 0;

double getOptionalNum (int stackIndex, double defaultValue) { return getArgCount()>stackIndex && isNum(stackIndex)? getNum(stackIndex) : defaultValue; }
bool getOptionalBool (int stackIndex, bool defaultValue) { return getArgCount()>stackIndex && isBool(stackIndex)? getBool(stackIndex) : defaultValue; }
std::string getOptionalString (int stackIndex, const std::string& defaultValue) { return getArgCount()>stackIndex && isString(stackIndex)? getString(stackIndex) : defaultValue; }
Handle getOptionalHandle  (int stackIndex, const Handle& defaultValue) { return getArgCount()>stackIndex? getHandle(stackIndex) : defaultValue; }

virtual void setNum (int stackIndex, double value) = 0;
virtual void setBool (int stackIndex, bool value) = 0;
virtual void setString (int stackIndex, const std::string& value) = 0;
virtual void setCString (int stackIndex, const char* value) = 0;
virtual void setBuffer (int stackIndex, const void* data, int length) = 0;
virtual void setRange (int stackIndex, const Range& value) = 0;
virtual void setNull (int stackIndex) = 0;
virtual void* setNewUserPointer (int stackIndex, size_t classId) = 0;
virtual void setHandle (int stackIndex, const Handle& handle) = 0;

virtual void pushNum (double value) = 0;
virtual void pushBool (bool value) = 0;
virtual void pushString (const std::string& value) = 0;
virtual void pushCString (const char* value) = 0;
virtual void pushBuffer (const void* data, int length) = 0;
virtual void pushRange (const Range& value) = 0;
virtual void pushNull () = 0;
virtual void pushNativeFunction (NativeFunction f) = 0;
virtual void pushNewForeignClass (const std::string& name, size_t classId, int nUserBytes, int nParents=0) = 0;
virtual void* pushNewUserPointer (size_t classId) = 0;
virtual void pushHandle (const Handle& handle) = 0;

virtual void pushCopy (int stackIndex = -1) = 0;
virtual void pop () = 0;

virtual void loadString (const std::string& source, const std::string& name="") = 0;
virtual void loadFile (const std::string& filename) = 0;

virtual void call (int nArgs) = 0;
virtual void callMethod (const std::string& name, int nArgs) = 0;
virtual void storeGlobal (const std::string& name) = 0;
virtual void loadGlobal (const std::string& name) = 0;
virtual void storeMethod (const std::string& name) = 0;
virtual void storeStaticMethod (const std::string& name) = 0;
virtual void storeDestructor ( void(*)(void*) ) = 0;

template<class T> inline const T* getBuffer (int stackIndex, int* length = nullptr) {
const T* re = reinterpret_cast<const T*>(getBufferV(stackIndex, length));
if (length) *length /= sizeof(T);
return re;
}

template<class T> T& getUserObject (int stackIndex) {
return *static_cast<T*>(getUserPointer(stackIndex));
}

template<class T> void setUserObject (int stackIndex, const T& obj) {
void* ptr = setNewUserPointer(stackIndex, typeid(T).hash_code());
new(ptr) T(obj);
}

template<class T, class... A> void emplaceUserObject (int stackIndex, A&&... args) {
void* ptr = setNewUserPointer(stackIndex, typeid(T).hash_code());
new(ptr) T(args...);
}

template<class T> inline void pushNewClass (const std::string& name, int nParents=0) { 
pushNewForeignClass(name, typeid(T).hash_code(), sizeof(T), nParents); 
}

template<class T>  inline void registerClass (const std::string& name, int nParents=0) {
pushNewClass<T>(name, nParents);
storeGlobal(name);
}

inline void registerFunction (const std::string& name, const NativeFunction& func) {
pushNativeFunction(func);
storeGlobal(name);
pop();
}

inline void registerMethod (const std::string& name, const NativeFunction& func) {
pushNativeFunction(func);
storeMethod(name);
pop();
}

inline void registerStaticMethod (const std::string& name, const NativeFunction& func) {
pushNativeFunction(func);
storeStaticMethod(name);
pop();
}

inline void registerProperty (const std::string& name, const NativeFunction& getter) {
registerMethod(name, getter);
}

inline void registerStaticProperty (const std::string& name, const NativeFunction& getter) {
registerStaticMethod(name, getter);
}

inline void registerProperty (const std::string& name, const NativeFunction& getter, const NativeFunction& setter) {
registerMethod(name, getter);
registerMethod(name+"=", setter);
}

inline void registerStaticProperty (const std::string& name, const NativeFunction& getter, const NativeFunction& setter) {
registerStaticMethod(name, getter);
registerStaticMethod(name+"=", setter);
}

template <class T, class... A> inline void registerConstructor ();
template <class T> inline void registerDestructor ();
};

struct VM {
typedef std::function<std::string(const std::string&, const std::string&)> PathResolverFn;
typedef std::function<std::string(const std::string&)> FileLoaderFn;
typedef std::function<void(const CompilationMessage&)> CompilationMessageFn;
typedef std::function<void(std::istream& in, std::ostream& out)> EncodingConversionFn;
typedef std::function<void(std::istream& in, std::ostream& out, int)> DecodingConversionFn;

enum Option {
VAR_DECL_MODE = 0, // Variable declaration mode
VAR_STRICT = 0, // Undefined variables are signaled and stop compilation. Recommanded option.
VAR_IMPLICIT, // Using an undefined variable cause it to be declared implicitly, as if the keyword var had been used
VAR_IMPLICIT_GLOBAL // Same as VAR_IMPLICIT except that the variable is implicitly declared global. Useful for interactive mode.
};

VM () = default;
VM (const VM&) = delete;
VM& operator= (const VM&) = delete;
virtual ~VM () = default;

virtual Fiber* createFiber () = 0;

virtual void setPathResolver (const PathResolverFn& fn) = 0;
virtual void setFileLoader (const FileLoaderFn& fn) = 0;
virtual void setCompilationMessageReceiver (const CompilationMessageFn& fn) = 0;
virtual int getOption (Option opt) = 0;
virtual void setOption (Option opt, int value = 1) = 0;

virtual void garbageCollect () = 0;

static VM* export createVM ();
static Fiber* export getActiveFiber ();
static EncodingConversionFn export getEncoder (const std::string& name);
static DecodingConversionFn export getDecoder (const std::string& name);
static void export registerEncoder (const std::string& name, const EncodingConversionFn& func);
static void export registerDecoder (const std::string& name, const DecodingConversionFn& func);
};

} // namespace QS
#endif
