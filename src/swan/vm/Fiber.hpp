#ifndef _____SWAN_FIBER_HPP_____
#define _____SWAN_FIBER_HPP_____
#include "Core.hpp"
#include "Value.hpp"
#include "Iterable.hpp"
#include "String.hpp"
#include "Closure.hpp"
#include "ExecutionStack.hpp"
#include "CatchPoint.hpp"
#include "CallFrame.hpp"
#include "ForeignInstance.hpp"
#include "Function.hpp"
#include "Allocator.hpp"
#include<string>
#include<vector>

struct QFiber: Swan::Fiber, QSequence  {
typedef execution_stack<QV, trace_allocator<QV>> Stack;
Stack stack;
std::vector<QCallFrame, trace_allocator<QCallFrame>> callFrames;
std::vector<QCatchPoint, trace_allocator<QCatchPoint>> catchPoints;
std::vector<struct Upvalue*, trace_allocator<Upvalue*>> openUpvalues;
QVM& vm;
QFiber* parentFiber;
FiberState state;

inline void returnValue (QV value) { stack.at(callFrames.back().stackBase) = value; }
inline void returnValue (const std::string& s) { returnValue(QV(vm,s)); }

inline QV& at (int i) {
return i>=0? stack.at(callFrames.back().stackBase+i) : *(stack.end() +i);
}

virtual inline int getArgCount () final override { return stack.size() - callFrames.back().stackBase; }
virtual inline Swan::VM& getVM () final override;

virtual inline bool isNum (int i) final override { return at(i).isNum(); }
virtual inline bool isBool  (int i) final override { return at(i).isBool(); }
virtual inline bool isString (int i) final override { return at(i).isString(); }
virtual bool isRange (int i) final override;
virtual inline bool isNull (int i) final override { return at(i).isNull(); }
virtual inline bool isUndefined  (int i) final override { return at(i).isUndefined(); }
virtual inline bool isNullOrUndefined (int i) final override { return at(i).isNullOrUndefined(); }
virtual bool isUserPointer (int i, size_t classId) final override;

template<class T> inline T& getObject (int i) { return *at(i).asObject<T>(); }
virtual inline double getNum (int i) final override {  return at(i).asNum(); }
virtual inline bool getBool (int i) final override { return at(i).asBool(); }
virtual inline std::string getString (int i) final override { return at(i).asString(); }
virtual inline const char* getCString (int i) final override { return at(i).asCString(); }
virtual inline const Swan::Range& getRange (int i) final override { return at(i).asRange(); }
virtual inline Swan::Fiber& getFiber (int i) final override { return getObject<QFiber>(i); }
virtual inline void* getUserPointer (int i) final override { return getObject<QForeignInstance>(i).userData; }
virtual inline Swan::Handle getHandle (int i) final override { return at(i).asHandle(); }

virtual inline double getOptionalNum (int i, double defaultValue) { return getArgCount()>i && isNum(i)? getNum(i) : defaultValue; }
virtual inline bool getOptionalBool (int i, bool defaultValue) { return getArgCount()>i && isBool(i)? getBool(i) : defaultValue; }
virtual inline std::string getOptionalString  (int i, const std::string& defaultValue) { return getArgCount()>i && isString(i)? getString(i) : defaultValue; }
virtual inline void* getOptionalUserPointer (int i, size_t classId, void* defaultValue = nullptr) { return getArgCount()>i && isUserPointer(i, classId)? getUserPointer(i) : defaultValue; }
virtual inline Swan::Handle getOptionalHandle  (int i, const Swan::Handle& defaultValue) { return getArgCount()>i && !isNullOrUndefined(i)? at(i).asHandle() : defaultValue; }

virtual double getOptionalNum (int stackIndex, const std::string& key, double defaultValue) final override;
virtual bool getOptionalBool (int stackIndex, const std::string& key, bool defaultValue) final override;
virtual std::string getOptionalString (int stackIndex, const std::string& key, const std::string& defaultValue) final override;
virtual void* getOptionalUserPointer (int stackIndex, const std::string& key, size_t classId, void* defaultValue = nullptr) final override;
virtual Swan::Handle getOptionalHandle  (int stackIndex, const std::string& key, const Swan::Handle& defaultValue) final override;

virtual std::vector<double> getNumList (int stackIndex);
virtual std::vector<std::string> getStringList (int stackIndex);

virtual inline void setNum (int i, double d) final override { at(i).d = d; }
virtual inline void setBool (int i, bool b) final override { at(i) = QV(b); }
virtual inline void setString  (int i, const std::string& s) final override;
virtual inline void setCString  (int i, const char* s) final override;
virtual void setRange  (int i, const Swan::Range& r) final override;
virtual inline void setNull (int i) final override { at(i) = QV::Null; }
virtual inline void setUndefined (int i) final override { at(i) = QV::UNDEFINED; }
virtual inline void setNativeFunction (int i, Swan::NativeFunction f) final override { at(i) = QV(reinterpret_cast<void*>(f), QV_TAG_NATIVE_FUNCTION); }
virtual void setStdFunction (int i, const std::function<void(Swan::Fiber&)>& func) final override;
virtual inline void setFiber (int i, Swan::Fiber& f) final override { at(i) = QV(static_cast<QFiber*>(&f)); }
virtual void* setNewUserPointer (int i, size_t id) final override;
virtual void setHandle (int i, const Swan::Handle& h) final override { at(i) = QV(h.value); }
inline void setObject (int i, QObject* obj) { at(i)=QV(obj); }
inline void setObject (int i, QObject& obj) { setObject(i, &obj); }

virtual inline void pushNum (double d) final override { stack.push_back(d); }
virtual inline void pushBool  (bool b) final override { stack.push_back(b); }
virtual inline void pushString (const std::string& s) final override;
virtual inline void pushCString (const char* s) final override;
virtual void pushRange (const Swan::Range& r) final override;
virtual inline void pushNull  () final override { stack.push_back(QV::Null); }
virtual inline void pushUndefined () final override { stack.push_back(QV::UNDEFINED); }
virtual inline void pushNativeFunction (Swan::NativeFunction f) final override { stack.push_back(QV(reinterpret_cast<void*>(f), QV_TAG_NATIVE_FUNCTION)); }
virtual inline void pushStdFunction (const std::function<void(Swan::Fiber&)>& f) final override ;
virtual inline void pushFiber (Swan::Fiber& f) final override { push(QV(static_cast<QFiber*>(&f))); }
virtual void pushNewForeignClass (const std::string& name, size_t id, int nUserBytes, int nParents=0) final override;
virtual void* pushNewUserPointer (size_t id) final override;
virtual inline void pushHandle (const Swan::Handle& h) final override { push(QV(h.value)); }
virtual inline void pushCopy (int i = -1) final override { stack.push_back(at(i)); }
virtual inline void swap (int i1 = -2, int i2 = -1) final override { std::swap(at(i1), at(i2)); }
virtual inline void setCopy (int i, int j) final override { at(i) = at(j); }
virtual inline void insertCopy (int i, int j) final override { stack.insert(&at(i), at(j)); }
virtual inline void pop () final override { stack.pop_back(); }
inline QV& top () { return *(stack.end() -1); }
inline QV& base () { return stack.at(callFrames.back().stackBase); }
inline void push (const QV& x) { stack.push_back(x); }
inline void pushCppCallFrame () { callFrames.push_back({ nullptr, nullptr, stack.size() }); }
inline void popCppCallFrame () { callFrames.pop_back(); }

inline QString* ensureString (QV& val);
inline QString* ensureString (int i) { return ensureString(at(i)); }

virtual void import (const std::string& baseFile, const std::string& requestedFile) final override;
virtual void storeImport (const std::string& name) final override;
virtual int loadString  (const std::string& source, const std::string& name="") final override;
virtual int loadFile (const std::string& filename) final override;
virtual void dumpBytecode (std::ostream& out, int count = 1) final override;
virtual void importAndDumpBytecode (const std::string& baseFile, const std::string& toImport, std::ostream& out) final override;
int loadBytecode (std::istream& in);
void saveBytecode (std::ostream& out, int count = 1);
int loadString (const std::string& source, const std::string& filename, const std::string& displayName, const QV& additionalContextVar = QV::UNDEFINED);

virtual void storeGlobal (const std::string& name, bool isConst=false) final override;
virtual void loadGlobal (const std::string& name) final override;
virtual void storeMethod (const std::string& name) final override;
virtual void storeStaticMethod (const std::string& name) final override;
virtual void storeDestructor ( void(*destr)(void*) ) final override;
virtual void call (int nArgs) final override;
virtual void callMethod (const std::string& name, int nArgs) final override;

QFiber (QVM& vm);
QFiber (QVM& vm0, QClosure& closure): QFiber(vm0) { callFrames.push_back({ &closure,  closure.func.bytecode.data() , 0 }); }
void adjustArguments (int nArgs, int nClosureArgs, bool vararg);
void storeMethod (int symbol);
void storeStaticMethod (int symbol);
QV loadMethod (QV& obj, int symbol);
void pushNewClass (int nParents, int nStaticFields, int nFields);
void  callSymbol (int symbol, int nArgs);
void callSuperSymbol (int symbol, int nArgs);
inline bool callFunc (QV& callable, int nArgs);
void callCallable (int nArgs);
inline void callClosure (QClosure& closure, int nArgs);
void callFiber (QFiber& f, int nArgs);

void unpackSequence ();
void loadPushClosure (QClosure* curClosure, uint_constant_index_t constantIndex);
Upvalue* captureUpvalue (int slot);
void closeUpvalues (int startSlot);
void adjustUpvaluePointers (const QV* oldPtr, const QV* newPtr);
void handleException (const std::exception& e);

FiberState run ();
virtual ~QFiber ();
virtual void release () final override;
virtual bool gcVisit () override;
virtual void* gcOrigin () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

template<class E, class... A> inline void error (const char* msg, A&&... args) {
throw E(format(msg, args...));
}

//Just because std::bad_function_call doesn't accept any argument
struct call_error: std::runtime_error {
call_error(const std::string& s): runtime_error(s) {}
};

#endif
