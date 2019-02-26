#ifndef _____SWAN_FIBER_HPP_____
#define _____SWAN_FIBER_HPP_____
#include "Core.hpp"
#include "Value.hpp"
#include "Sequence.hpp"
#include "String.hpp"
#include "Closure.hpp"
#include "ExecutionStack.hpp"
#include "CatchPoint.hpp"
#include "CallFrame.hpp"
#include "Mutex.hpp"
#include "ForeignInstance.hpp"
#include "Function.hpp"
#include<string>
#include<vector>

struct QFiber: Swan::Fiber, QSequence  {
typedef execution_stack<QV> Stack;
static thread_local QFiber* curFiber;
Stack stack;
std::vector<QCallFrame> callFrames;
std::vector<QCatchPoint> catchPoints;
std::vector<struct Upvalue*> openUpvalues;
QVM& vm;
QFiber* parentFiber;
Mutex mutex;
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
virtual bool isBuffer (int i) final override;
virtual inline bool isNull (int i) final override { return at(i).isNull(); }
virtual bool isUserPointer (int i, size_t classId) final override;

template<class T> inline T& getObject (int i) { return *at(i).asObject<T>(); }
virtual inline double getNum (int i) final override {  return at(i).asNum(); }
virtual inline bool getBool (int i) final override { return at(i).asBool(); }
virtual inline std::string getString (int i) final override { return at(i).asString(); }
virtual inline const char* getCString (int i) final override { return at(i).asCString(); }
virtual inline const Swan::Range& getRange (int i) final override { return at(i).asRange(); }
virtual const void* getBufferV (int i, int* length = nullptr) final override;
virtual inline void* getUserPointer (int i) final override { return getObject<QForeignInstance>(i).userData; }
virtual inline Swan::Handle getHandle (int i) final override { return at(i).asHandle(); }

virtual inline double getOptionalNum (int i, double defaultValue) { return getArgCount()>i && isNum(i)? getNum(i) : defaultValue; }
virtual inline bool getOptionalBool (int i, bool defaultValue) { return getArgCount()>i && isBool(i)? getBool(i) : defaultValue; }
virtual inline std::string getOptionalString  (int i, const std::string& defaultValue) { return getArgCount()>i && isString(i)? getString(i) : defaultValue; }
virtual inline Swan::Handle getOptionalHandle  (int i, const Swan::Handle& defaultValue) { return getArgCount()>i && !isNull(i)? at(i).asHandle() : defaultValue; }

virtual double getOptionalNum (int stackIndex, const std::string& key, double defaultValue) final override;
virtual bool getOptionalBool (int stackIndex, const std::string& key, bool defaultValue) final override;
virtual std::string getOptionalString (int stackIndex, const std::string& key, const std::string& defaultValue) final override;
virtual Swan::Handle getOptionalHandle  (int stackIndex, const std::string& key, const Swan::Handle& defaultValue) final override;

virtual inline void setNum (int i, double d) final override { at(i).d = d; }
virtual inline void setBool (int i, bool b) final override { at(i) = QV(b); }
virtual inline void setString  (int i, const std::string& s) final override;
virtual inline void setCString  (int i, const char* s) final override;
virtual inline void setBuffer  (int i, const void* data, int length) final override;
virtual void setRange  (int i, const Swan::Range& r) final override;
virtual inline void setNull (int i) final override { at(i) = QV(); }
virtual void* setNewUserPointer (int i, size_t id) final override;
virtual void setHandle (int i, const Swan::Handle& h) final override { at(i) = QV(h.value); }
inline void setObject (int i, QObject* obj) { at(i)=QV(obj); }
inline void setObject (int i, QObject& obj) { setObject(i, &obj); }

virtual inline void pushNum (double d) final override { stack.push_back(d); }
virtual inline void pushBool  (bool b) final override { stack.push_back(b); }
virtual inline void pushString (const std::string& s) final override;
virtual inline void pushCString (const char* s) final override;
virtual inline void pushBuffer  (const void* data, int length) final override;
virtual void pushRange (const Swan::Range& r) final override;
virtual inline void pushNull  () final override { stack.push_back(QV()); }
virtual inline void pushNativeFunction (Swan::NativeFunction f) final override { stack.push_back(QV(reinterpret_cast<void*>(f), QV_TAG_NATIVE_FUNCTION)); }
virtual void pushNewForeignClass (const std::string& name, size_t id, int nUserBytes, int nParents=0) final override;
virtual void* pushNewUserPointer (size_t id) final override;
virtual inline void pushHandle (const Swan::Handle& h) final override { push(QV(h.value)); }
virtual inline void pushCopy (int i = -1) final override { stack.push_back(at(i)); }
virtual inline void pop () final override { stack.pop_back(); }
inline QV& top () { return at(-1); }
inline void push (const QV& x) { stack.push_back(x); }
inline void pushCppCallFrame () { callFrames.push_back({ nullptr, nullptr, stack.size() }); }
inline void popCppCallFrame () { callFrames.pop_back(); }

inline QString* ensureString (QV& val);
inline QString* ensureString (int i) { return ensureString(at(i)); }

virtual void lock () final override { mutex.lock(); }
virtual void unlock () final override { mutex.unlock(); }
virtual void import (const std::string& baseFile, const std::string& requestedFile) final override;
virtual void storeImport (const std::string& name) final override;
virtual int loadString  (const std::string& source, const std::string& name="") final override;
virtual int loadFile (const std::string& filename) final override;
virtual void dumpBytecode (std::ostream& out, int count = 1) final override;
int loadBytecode (std::istream& in);
void saveBytecode (std::ostream& out, int count = 1);

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
QV loadMethod (QV& obj, int symbol);
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

#endif
