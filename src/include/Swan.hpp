#ifndef _____SWAN_HPP_____
#define _____SWAN_HPP_____

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
#include<iosfwd>

namespace Swan {

struct VM;
struct Fiber;
typedef void(*NativeFunction)(Fiber&);

/** Exception type thrown when a compilation or runtime error occurs */
struct ScriptException: std::exception {
std::string msg;
ScriptException (const std::string& m): msg(m) {}
const char* what () const noexcept final override { return msg.c_str(); }
~ScriptException () = default;
};

/** Exception thrown when an error occurrs during the compilation of a swan script */
struct CompilationException: ScriptException {
bool incomplete;
inline bool isIncomplete () { return incomplete; }
inline CompilationException (bool ic): ScriptException("Compilation failed"), incomplete(ic) {}
inline CompilationException (const std::string& msg, bool ic): ScriptException(msg), incomplete(ic) {}
~CompilationException () = default;
};

/** Structure grouping information about a compiler message */
struct CompilationMessage {
/** Enumeration of possible kinds of compiler messages */
enum Kind {
/** Error: the compilation is totally impossible when an error has occurred */
ERROR,
/** Warning: the compilation can still continue when a warning occurrs, but they can reveal a possible bug or code smell in swan, or the use of potentially unsafe or deprecated feature */
WARNING,
/** Info: they are purely indicative, but can still reveal important weaknesses sometimes */
INFO
};
Kind kind;
/** Text of the compiler message, e.g. "undefined variable" */
std::string message;
/** The nearest token or keyword that triggered the message */
std::string token;
/** The file name currently compiling */
std::string file;
/** The line number in the file */
int line;
/** The column number in the file, i.e. the number of characters from the beginning of the line */
int column;
};

/** Structure holding information about a stack traceback line. Stack traces are generated when a runtime error occurrs and allow to locate the specific context of the error. */
struct StackTraceElement {
/** Name of the function where the runtime error occurred */
std::string function;
/** File name containing the function. This information may not be available if the script has been compiled without debug info */
std::string file;
/** Line number in the source code where the error occurred. This information may not be available if the script has been compiled without debug info */
int line;
};

/** Exception thrown when a runtime error occurrs when running swan code */
struct RuntimeException: ScriptException {
/** Lines of stack traceback describing the context of the error */
std::vector<StackTraceElement> stackTrace;
/** The type of exception. May contain a C++ exception type name e.g. std::logic_error, or a general category of exception e.g. invalid argument */
std::string type;
/** Error code associated with the type */
int code;
inline RuntimeException (const std::string& cat, const std::string& msg, int c): ScriptException(msg), type(cat), code(c) {}
/** Generate a string containing the entire stack trace */
std::string export getStackTraceAsString ();
};

/** Structure describing a range */
struct Range {
/** Starting value of the range */
double start;
/** Final value of the range */
double end;
/** The step, a.k.a increment between two values in the range */
double step;
/** Whether or not the final value is included in the range */
bool inclusive;
/** Standard constructor specifying start, end, step, and inclusivity defaulted to false (exclusive range) */
Range (double s, double e, double p, bool i=false): start(s), end(e), step(p), inclusive(i) {}
/**  Standard constructor specifying start and end values and inclusivity defaulted to false (exclusive range). The step is set to 1 or -1 depending on if end>start or end<start. */
Range (double s, double e, bool i=false): Range(s, e, e>=s?1:-1, i) {}
/** Standard constructor specifying end value and inclusivity defaulted to false (exclusive range). Start is set to 0 and step to 1 unless end is negative */
Range (double e, bool i=false): Range(0, e, i) {}
/** Given a fixed number of elements (the length) e.g. in a array, set up the actual beginning and final values between 0 and length -1. This handles negative or out-of-bounds list indices that are allowed in swan, for example, and make sure  they always remain valid in C++ where out-of-bound indices are forbidden. */
inline void makeBounds (int length, int& begin, int& finish) const {
begin = start<0? length+start : start;
finish = end<0? length+end : end;
if (inclusive) finish++;
if (begin<0) begin=0; if (finish<0) finish=0;
if (begin>length) begin=length; if (finish>length) finish=length;
if (finish<begin) finish=begin;
}
};

struct export Handle {
uint64_t value;
export Handle ();
Handle (const Handle&) = default;
export Handle (Handle&&);
Handle& operator= (const Handle&) = default;
Handle& export operator= (Handle&&);
export ~Handle ();
};

/** Fiber of swan script execution */
struct Fiber {

protected:
Fiber () = default;
virtual ~Fiber () = default;

public:
/** Fiber is not copiable */
Fiber (const Fiber&) = delete;
/** Fiber is not copiable */
Fiber (Fiber&&) = delete;
/** Fiber is not copiable */
Fiber& operator= (const Fiber&) = delete;
/** Fiber is not copiable */
Fiber& operator= (Fiber&&) = delete;

/** Return the number of arguments currentely on the stack frame */
virtual int getArgCount () = 0;

/** Return a reference to the Swan VM on which this fiber is running */
virtual VM& getVM () = 0;

/** Check if the element at stackIndex is a Bool value */
virtual bool isBool (int stackIndex) = 0;
/** Check if the element at stackIndex is a Num value */
virtual bool isNum (int stackIndex) = 0;
/** Check if the element at stackIndex is a String value */
virtual bool isString (int stackIndex) = 0;
/** Check if the element at stackIndex is a Range value */
virtual bool isRange (int stackIndex) = 0;
/** Check if the element at stackIndex is a Buffer value */
virtual bool isBuffer (int stackIndex) = 0;
/** Check if the element at stackIndex is a Null value */
virtual bool isNull (int stackIndex) = 0;
/** Check if the element at stackIndex is a pointer to an object of the type specified by its ID */
virtual bool isUserPointer (int stackIndex, size_t classId) = 0;

/** Return the value of the Num object at the given stack index. An undefined value is returned in case the requested element isn't of the required type. */
virtual double getNum (int stackIndex) = 0;
/** Return the value of the Bool object at the given stack index. An undefined value is returned in case the requested element isn't of the required type. */
virtual bool getBool (int stackIndex) = 0;
/** Return the value of the String object at the given stack index, as a C++ std::string object. The behavior is undefined in case the requested element isn't of the required type. */
virtual std::string getString (int stackIndex) = 0;
/** Return the value of the String object at the given stack index, as a C-style null-terminated string. The behavior is undefined in case the requested element isn't of the required type. */
virtual const char* getCString (int stackIndex) = 0;
/** Return the value of the Range object at the given stack index. The behavior is undefined in case the requested element isn't of the required type. */
virtual const Range& getRange (int stackIndex) = 0;
/** Return a pointer to the data stored in the Buffer object at the given stack index, and optionally store the size of the buffer in length. The size isn't stored if length==nullptr. The behavior is undefined in case the requested element isn't of the required type. */
virtual const void* getBufferV (int stackIndex, int* length = nullptr) = 0;
/** Return a pointer to the object at the given stack index, if it is of any registered type (non built-in Swan type). The behavior is undefined in case the requested element isn't of the required type. */
virtual void* getUserPointer (int stackIndex) = 0;
/** Return an handle to the Swan object at the given stack index. This allows you to use that handle later on, for example to call Swan callbacks. */
virtual Handle getHandle (int stackIndex) = 0;

/** Return the value of the Num object at the given stack index, or defaultValue if there isn't that many stack elements or if the given stack element isn't of an appropriate type */
inline double getOptionalNum (int stackIndex, double defaultValue) { return getArgCount()>stackIndex && isNum(stackIndex)? getNum(stackIndex) : defaultValue; }
/** Return the value of the Bool object at the given stack index, or defaultValue if there isn't that many stack elements or if the given stack element isn't of an appropriate type */
inline bool getOptionalBool (int stackIndex, bool defaultValue) { return getArgCount()>stackIndex && isBool(stackIndex)? getBool(stackIndex) : defaultValue; }
/** Return the value of the String object at the given stack index as a C++ std::string object, or defaultValue if there isn't that many stack elements or if the given stack element isn't of an appropriate type */
inline std::string getOptionalString (int stackIndex, const std::string& defaultValue) { return getArgCount()>stackIndex && isString(stackIndex)? getString(stackIndex) : defaultValue; }
/** Return an handle to the object at the given stack index, or defaultValue if there isn't that many stack elements or if the given stack element isn't of an appropriate type */
inline Handle getOptionalHandle  (int stackIndex, const Handle& defaultValue) { return getArgCount()>stackIndex? getHandle(stackIndex) : defaultValue; }

/** Look for a Num object at the given stack index, or inside a Map at index -1 with the given key. Return the value of that Num object if found, otherwise defaultValue */
virtual double getOptionalNum (int stackIndex, const std::string& key, double defaultValue) = 0;
/** Look for a Bool object at the given stack index, or inside a Map at index -1 with the given key. Return the value of that Bool object if found, otherwise defaultValue */
virtual bool getOptionalBool (int stackIndex, const std::string& key, bool defaultValue) = 0;
/** Look for a String object at the given stack index, or inside a Map at index -1 with the given key. Return the value of that String object as a C++ std::string if found, otherwise defaultValue */
virtual std::string getOptionalString (int stackIndex, const std::string& key, const std::string& defaultValue) = 0;
/** Look for any object at the given stack index, or inside a Map at index -1 with the given key. Return an handle to that object if found, otherwise defaultValue */
virtual Handle getOptionalHandle  (int stackIndex, const std::string& key, const Handle& defaultValue) = 0;

/** Look for a Num object inside a Map at index -1 with the given key. Return the value of that Num object if found, otherwise defaultValue */
inline double getOptionalNum (const std::string& key, double defaultValue) { return getOptionalNum(-1, key, defaultValue); }
/** Look for a Bool object inside a Map at index -1 with the given key. Return the value of that Bool object if found, otherwise defaultValue */
inline bool getOptionalBool  (const std::string& key, bool defaultValue) { return getOptionalBool(-1, key, defaultValue); }
/** Look for a String object inside a Map at index -1 with the given key. Return the value of that String object as a C++ std::string if found, otherwise defaultValue */
inline std::string getOptionalString (const std::string& key, const std::string& defaultValue) { return getOptionalString(-1, key, defaultValue); }
/** Look for any object inside a Map at index -1 with the given key. Return an handle to that object if found, otherwise defaultValue */
inline Handle getOptionalHandle (const std::string& key, const Handle& defaultValue) { return getOptionalHandle(-1, key, defaultValue); }

/** Extract a list of Num from a Tuple, List or Grid present at the given stack index and return it as a vector of double. The behavior is undefined if the designated element is of an uncompatible type. */
virtual std::vector<double> getNumList (int stackIndex) = 0;
/** Extract a list of String from a Tuple, List or Grid present at the given stack index and return it as a vector of std::string. The behavior is undefined if the designated element is of an uncompatible type. */
virtual std::vector<std::string> getStringList (int stackIndex) = 0;

/** Replace the element at the given stack index by a Num with the specified value */
virtual void setNum (int stackIndex, double value) = 0;
/** Replace the element at the given stack index by a Bool with the specified value */
virtual void setBool (int stackIndex, bool value) = 0;
/** Replace the element at the given stack index by a String with the specified value */
virtual void setString (int stackIndex, const std::string& value) = 0;
/** Replace the element at the given stack index by a String with the specified value */
virtual void setCString (int stackIndex, const char* value) = 0;
/** Replace the element at the given stack index by a Buffer with the specified content. The data is copied. */
virtual void setBuffer (int stackIndex, const void* data, int length) = 0;
/** Replace the element at the given stack index by a Range with the specified value */
virtual void setRange (int stackIndex, const Range& value) = 0;
/** Replace the element at the given stack index by a null value */
virtual void setNull (int stackIndex) = 0;
/** Replace the element at the specified stack index by a new object of the type given by its ID. Return a pointer to an uninitialized block of memory where the data associated to the object can be copied. */
virtual void* setNewUserPointer (int stackIndex, size_t classId) = 0;
/** Replace the element at the given stack index by the value stored in the handle */
virtual void setHandle (int stackIndex, const Handle& handle) = 0;

/** Push a Num object at the back of the stack */
virtual void pushNum (double value) = 0;
/** Push a Bool object at the back of the stack */
virtual void pushBool (bool value) = 0;
/** Push a String object at the back of the stack */
virtual void pushString (const std::string& value) = 0;
/** Push a String object at the back of the stack */
virtual void pushCString (const char* value) = 0;
/** Push a Buffer object at the back of the stack. The data of the buffer is copied. */
virtual void pushBuffer (const void* data, int length) = 0;
/** Push a Range object at the back of the stack */
virtual void pushRange (const Range& value) = 0;
/** Push a null at the back of the stack */
virtual void pushNull () = 0;
/** Push a Function object at the back of the stack */
virtual void pushNativeFunction (NativeFunction f) = 0;
/** Push a new Class object at the back of the stack. */
virtual void pushNewForeignClass (const std::string& name, size_t classId, int nUserBytes, int nParents=0) = 0;
/** Push a new object of the type given by its ID at the back of the stack. Return a pointer to an uninitialized block of memory where the data associated to the object can be copied. */
virtual void* pushNewUserPointer (size_t classId) = 0;
/** Push the value of an handle at the back of the stack */
virtual void pushHandle (const Handle& handle) = 0;

/** Push a copy of the element at given stack index. The default stackIndex=-1 duplicates the last element. */
virtual void pushCopy (int stackIndex = -1) = 0;
/** Swap the two designated elements at given stack indices. The defaults -2 and -1 swaps the two last elements. */
virtual void swap (int stackIndex1 = -2, int stackIndex2 = -1) = 0;
/** Pop the last stack element. The behavior is undefined if the stack is currently empty. */
virtual void pop () = 0;

/** Loads the given Swan source code and compile it. Return the number of Function objects pushed to the stack. The name is used in compilation or runtime error messages. */
virtual int loadString (const std::string& source, const std::string& name="") = 0;
/** Load the Swan source code in the file given and compile it. Return the number of Function objects pushed to the stack. */
virtual int loadFile (const std::string& filename) = 0;
/** Dump the count last elements of the stack into a binary stream. This is used to save bytecode in files.  */
virtual void dumpBytecode (std::ostream& out, int count = 1) = 0;
/** Import the given module, as if the import Swan instruction had been called from baseFile. */
virtual void import (const std::string& baseFile, const std::string& module) = 0;
/** Store the object at stack index -1 into the import list with the given name. Calling import with that name with later return the object stored. This can be used to provide built-in imports. */
virtual void storeImport (const std::string& name) = 0;
/** Import the given module originating from baseFile, and then dumps it in a binary stream. This is used to store bytecode into files. */
virtual void importAndDumpBytecode (const std::string& baseFile, const std::string& module, std::ostream& out) = 0;

/** Call a Swan function with the given number of arguments. The function to call, and then the arguments, must first be pushed on the stack. The function and its arguments are poped from the stack. */
virtual void call (int nArgs) = 0;
/** Call a method with the specified name and given number of arguments. The object on which to call the method (i.e. this/self), and then the arguments, must first be pushed on the stack. The this/self as well as the arguments are poped form the stack. */
virtual void callMethod (const std::string& name, int nArgs) = 0;
/** Store the object at stack index -1 as a global variable with the given name */
virtual void storeGlobal (const std::string& name, bool isConst=false) = 0;
/** Push the value of the global variable given by its name onto the stack */
virtual void loadGlobal (const std::string& name) = 0;
/** Store a method to a class. The class on which to store the method, and then the method itself as a Function object, must first be pushed on the stack. */
virtual void storeMethod (const std::string& name) = 0;
/** Store a static method to a class. The class on which to store the method, and then the method itself as a Function object, must first be pushed on the stack. */
virtual void storeStaticMethod (const std::string& name) = 0;
/** Store a destructor to a class. The class on which to store the destructor must be present at the back of the stack. */
virtual void storeDestructor ( void(*)(void*) ) = 0;

/** Return a pointer to the data stored in the Buffer object at the given stack index, and optionally store the size of the buffer in length. The size isn't stored if length==nullptr. The behavior is undefined in case the requested element isn't of the required type. */
template<class T> inline const T* getBuffer (int stackIndex, int* length = nullptr) {
const T* re = reinterpret_cast<const T*>(getBufferV(stackIndex, length));
if (length) *length /= sizeof(T);
return re;
}

/** Check if the element at stack index is of the templated type given. The type must of course first be registered. */
template<class T> inline bool isUserObject (int stackIndex) {
return isUserPointer(stackIndex, typeid(T).hash_code());
}

/** Return a reference pointing to the object at the given stack index, if it is of any registered type (non built-in Swan type). The behavior is undefined in case the requested element isn't of the required type. */
template<class T> inline T& getUserObject (int stackIndex) {
return *static_cast<T*>(getUserPointer(stackIndex));
}

/** Replace the element at stack index by an object of a registered type. The object is copied. T must be copiable and the copy constructor must be accessible. */
template<class T> inline void setUserObject (int stackIndex, const T& obj) {
void* ptr = setNewUserPointer(stackIndex, typeid(T).hash_code());
new(ptr) T(obj);
}

/** Replace the element at stack index by an object of a registered type. The stored object on the stack is constructed in place.  */
template<class T, class... A> inline void emplaceUserObject (int stackIndex, A&&... args) {
void* ptr = setNewUserPointer(stackIndex, typeid(T).hash_code());
new(ptr) T(args...);
}

/** Create a new Swan class with the given name and push it at the back of the stack. Parents, if any, must be pushed first. */
template<class T> inline void pushNewClass (const std::string& name, int nParents=0) { 
pushNewForeignClass(name, typeid(T).hash_code(), sizeof(T), nParents); 
}

/** Register a new C++ class for use in Swan. It creates a Swan Class object which is pused on the stack, and its name also become a global variable allowing to create instances in Swan. Parents, if any, must be pushed first. */
template<class T>  inline void registerClass (const std::string& name, int nParents=0) {
pushNewClass<T>(name, nParents);
storeGlobal(name);
}

/** Register a C++ function for use in Swan. The given name become a global variable allowing to call the function. */
inline void registerFunction (const std::string& name, const NativeFunction& func, bool isConst=false) {
pushNativeFunction(func);
storeGlobal(name, isConst);
pop();
}

/** Store a C++ function as a Swan method with the given name. The class on which the method has to be stored must be pushed first. */
inline void registerMethod (const std::string& name, const NativeFunction& func) {
pushNativeFunction(func);
storeMethod(name);
pop();
}

/** Store a C++ function as a Swan static method with the given name. The class on which the method has to be stored must be pushed first. */
inline void registerStaticMethod (const std::string& name, const NativeFunction& func) {
pushNativeFunction(func);
storeStaticMethod(name);
pop();
}

/** Store a C++ function as a Swan getter method with the given name. The class on which the method has to be stored must be pushed first. */
inline void registerProperty (const std::string& name, const NativeFunction& getter) {
registerMethod(name, getter);
}

/** Store a C++ function as a Swan static getter method with the given name. The class on which the method has to be stored must be pushed first. */
inline void registerStaticProperty (const std::string& name, const NativeFunction& getter) {
registerStaticMethod(name, getter);
}

/** Store a couple of C++ functions as a Swan getter and setter methods with the given name. The class on which the methods has to be stored must be pushed first. */
inline void registerProperty (const std::string& name, const NativeFunction& getter, const NativeFunction& setter) {
registerMethod(name, getter);
registerMethod(name+"=", setter);
}

/** Store a couple of C++ functions as a Swan static getter and setter methods with the given name. The class on which the methods has to be stored must be pushed first. */
inline void registerStaticProperty (const std::string& name, const NativeFunction& getter, const NativeFunction& setter) {
registerStaticMethod(name, getter);
registerStaticMethod(name+"=", setter);
}

/** Register a new global variable for use in Swan */
inline void registerGlobal (const std::string& name, double value, bool isConst=false) {
pushNum(value);
storeGlobal(name, isConst);
}

/** Register a new global variable for use in Swan */
inline void registerGlobal (const std::string& name, const std::string& value, bool isConst=false) {
pushString(value);
storeGlobal(name, isConst);
}

/** Register a global constant for use in Swan */
inline void registerConst (const std::string& name, double value, bool isConst=true) {
pushNum(value);
storeGlobal(name, isConst);
}

/** Register a global constant for use in Swan */
inline void registerConst (const std::string& name, const std::string& value, bool isConst=true) {
pushString(value);
storeGlobal(name, isConst);
}

/** Store a C++ construtor as a Swan constructor method. The class on which to store the constructor method must be pushed first. */
template <class T, class... A> inline void registerConstructor ();

/** Store a C++ destructor as a Swan destructor method. The class on which to store the destructor method must be pushed first. */
template <class T> inline void registerDestructor ();
};

/** The class representing a Swan virtual machine  on which scripts will be run */
struct VM {
enum ImportHookState { IMPORT_REQUEST, BEFORE_IMPORT, BEFORE_RUN, AFTER_RUN };
typedef std::function<bool(Swan::Fiber&, const std::string&, ImportHookState, int)> ImportHookFn;
typedef std::function<std::string(const std::string&, const std::string&)> PathResolverFn;
typedef std::function<std::string(const std::string&)> FileLoaderFn;
typedef std::function<void(const CompilationMessage&)> CompilationMessageFn;
typedef std::function<void(std::istream& in, std::ostream& out)> EncodingConversionFn;
typedef std::function<void(std::istream& in, std::ostream& out, int)> DecodingConversionFn;

enum Option {
VAR_DECL_MODE = 0, /// Variable declaration mode
VAR_STRICT = 0, /// Undefined variables are signaled and stop compilation. Recommanded option.
VAR_IMPLICIT, /// Using an undefined variable cause it to be declared implicitly, as if the keyword var had been used
VAR_IMPLICIT_GLOBAL, /// Same as VAR_IMPLICIT except that the variable is implicitly declared global. Useful for interactive mode.
COMPILATION_DEBUG_INFO = 1, /// compile with debug info
GC_TRESHHOLD_FACTOR = 2, /// Increase multiplier in GC treshhold at each GC cycle. Minimum 110%, Default: 200%
GC_TRESHHOLD = 3 /// Treshhold memory usage at which to trigger the GC. Minimum: 64 KB, Default: 64 KB
};

protected: 
VM () = default;
virtual ~VM () = default;

public: 
/** Swan VM objects aren't copiable */
VM (const VM&) = delete;
/** Swan VM objects aren't copiable */
VM (VM&&) = delete;
/** Swan VM objects aren't copiable */
VM& operator= (const VM&) = delete;
/** Swan VM objects aren't copiable */
VM& operator= (VM&&) = delete;

/** Return the currently running Fiber on this VM */
virtual Fiber& getActiveFiber () = 0;
/** Lock the VM to prevent other threads from using it */
virtual void lock () = 0;
/** Unlock the VM, allowing other threads to use it */
virtual void unlock () = 0;
/** Destroy the VM and free all associated Swan objects */
virtual void destroy () = 0;

/** Return the path resolver currently in use */
virtual const PathResolverFn& getPathResolver () = 0;
/** Set a path resolver to be used on import */
virtual void setPathResolver (const PathResolverFn& fn) = 0;
/** Return the file laoder currently in use */
virtual const FileLoaderFn& getFileLoader () = 0;
/** Set a file loader to be used on import */
virtual void setFileLoader (const FileLoaderFn& fn) = 0;
/** Return the receiver of compilation messages currently in use */
virtual const CompilationMessageFn& getCompilationMessageReceiver () = 0;
/** Set the compilation messages receiver to use when errors have to be reported */
virtual void setCompilationMessageReceiver (const CompilationMessageFn& fn) = 0;
/** Return the import hook currently in use */
virtual const ImportHookFn& getImportHook () = 0;
/** Set an import hook to be used on import */
virtual void setImportHook (const ImportHookFn& fn) = 0;
/** Get the current value of a given VM option */
virtual int getOption (Option opt) = 0;
/** Change the value of a given VM option */
virtual void setOption (Option opt, int value = 1) = 0;

/** Run the garbage collector */
virtual void garbageCollect () = 0;

/** Create a new instance of the Swan VM */
static VM& export create ();

static EncodingConversionFn export getEncoder (const std::string& name);
static DecodingConversionFn export getDecoder (const std::string& name);
static void export registerEncoder (const std::string& name, const EncodingConversionFn& func);
static void export registerDecoder (const std::string& name, const DecodingConversionFn& func);
};

template<class T> struct ScopeLocker {
T& ref;
inline ScopeLocker (T& x): ref(x) { ref.lock(); }
inline ~ScopeLocker () { ref.unlock(); }
};

template<class T> struct ScopeUnlocker {
T& ref;
inline ScopeUnlocker (T& x): ref(x) { ref.unlock(); }
inline ~ScopeUnlocker () { ref.lock(); }
};

} // namespace Swan
#endif
