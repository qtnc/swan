#ifndef _____SWAN_CLASS_HPP_____
#define _____SWAN_CLASS_HPP_____
#include "Object.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include "Array.hpp"
#include "../../include/cpprintf.hpp"

struct ClassGCInfo {
bool(*gcVisit)(QObject*);
size_t(*gcMemSize)(QObject*);
void*(*gcOrigin)(QObject*);
void(*gcDestroy)(QObject*);
bool(*join)(QObject*, struct QFiber&, const std::string&, std::string&);
bool(*copyInto)(QObject*, QFiber&, CopyVisitor&);
int(*getLength)(QObject*);
};

template<class T> void baseGCDestroy (QObject*  x) {
static_cast<T*>(x)->~T();
}

template<class T> bool baseGCVisit (QObject*  x) {
return static_cast<T*>(x)->gcVisit();
}

template<class T> size_t baseGCMemSize (QObject* x) {
return static_cast<T*>(x) ->getMemSize();
}

template<class T> void* baseGCOrigin (QObject* x) {
return static_cast<T*>(x);
}

template<class T> bool baseJoin (QObject* obj, struct QFiber& f, const std::string& delim, std::string& out) {
return static_cast<T*>(obj) ->join(f, delim, out);
}

template<class T> bool baseCopyInto  (QObject* obj, struct QFiber& f, CopyVisitor& out) {
return static_cast<T*>(obj) ->copyInto(f, out);
}

template<class T> int baseGetLength  (QObject* x) {
return static_cast<T*>(x) ->getLength();
}

template<class T> ClassGCInfo* baseClassGCInfo (bool vls=false) {
static ClassGCInfo info = { baseGCVisit<T>, baseGCMemSize<T>, baseGCOrigin<T>, baseGCDestroy<T>, baseJoin<T>, baseCopyInto<T>, baseGetLength<T>     };
return &info;
}

struct QClass: QObject {
QVM& vm;
QClass* parent;
c_string name;
std::vector<QV, trace_allocator<QV>> methods;
ClassGCInfo* gcInfo;
uint16_t nFields;
bool nonInheritable :1, foreign :1;
QV staticFields[0];

QClass (QVM& vm, QClass* type, QClass* parent, const std::string& name, uint16_t nFields, bool nonInheritable);
QClass* copyParentMethods ();
QClass* mergeMixinMethods (QClass* mixin);
QClass* bind (const std::string& methodName, QNativeFunction func);
QClass* bind (const std::string& methodName, QNativeFunction func, const char* typeInfo);
QClass* bind (int symbol, const QV& value);
template<class T> inline QClass* assoc (bool vls = false) { gcInfo = baseClassGCInfo<T>(vls);  return this; }
inline bool isSubclassOf (QClass* cls) { return this==cls || (parent && parent->isSubclassOf(cls)); }
inline QV findMethod (int symbol) {
QV re = symbol>=methods.size()? QV::UNDEFINED : methods[symbol];
if (re.isNullOrUndefined() && parent) return parent->findMethod(symbol);
else return re;
}
static QClass* create (QVM& vm, QClass* type, QClass* parent, const std::string& name, uint16_t nStaticFields=0, uint16_t nFields=0);
static QClass* createNonInheritable (QVM& vm, QClass* type, QClass* parent, const std::string& name);

QObject* instantiate ();
~QClass () = default;
bool gcVisit ();
inline size_t getMemSize ()  { return sizeof(*this) + sizeof(QV) * type->nFields; }
};

#endif
