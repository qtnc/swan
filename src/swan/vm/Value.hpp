#ifndef ____SWAN_VALUE_HPP_____
#define ____SWAN_VALUE_HPP_____
#include "../../include/Swan.hpp"
#include "Core.hpp"
#include "Object.hpp"
#include<string>

union QV {
uint64_t i;
double d;

std::string print () const;

inline explicit QV(): i(QV_UNDEFINED) {}
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
inline bool isUndefined () const { return i==QV_UNDEFINED; }
inline bool isNull () const { return i==QV_NULL; }
inline bool isNullOrUndefined () const { return isUndefined() || isNull(); }
inline bool isTrue () const { return i==QV_TRUE; }
inline bool isFalse () const { return i==QV_FALSE; }
inline bool isFalsy () const { return isFalse() || isNullOrUndefined(); }
inline bool isBool () const { return i==QV_TRUE || i==QV_FALSE; }
inline bool isObject () const { return (i&QV_NEG_NAN)==QV_NEG_NAN && i!=QV_NEG_NAN; }
inline bool isNum () const { return (i&QV_NAN)!=QV_NAN || i==QV_NAN || i==QV_NEG_NAN; }
inline bool isInteger () const { return isNum() && static_cast<int>(d)==d; }
inline bool isInt8 () const { return isInteger() && d>=-127 && d<=127; }
inline bool isClosure () const { return hasTag(QV_TAG_CLOSURE); }
inline bool isNormalFunction () const { return hasTag(QV_TAG_NORMAL_FUNCTION); }
inline bool isNativeFunction () const { return hasTag(QV_TAG_NATIVE_FUNCTION); }
inline bool isBoundFunction () const { return hasTag(QV_TAG_BOUND_FUNCTION); }
inline bool isStdFunction () const { return hasTag(QV_TAG_STD_FUNCTION); }
inline bool isGenericSymbolFunction () const { return hasTag(QV_TAG_GENERIC_SYMBOL_FUNCTION); }
inline bool isFiber () const { return hasTag(QV_TAG_FIBER); }
inline bool isCallable () const { return isClosure() || isNativeFunction() || isFiber() || isGenericSymbolFunction() || isBoundFunction() || isStdFunction() || isNormalFunction(); }
inline bool isString () const { return hasTag(QV_TAG_STRING); }
inline bool isOpenUpvalue () const { return hasTag(QV_TAG_OPEN_UPVALUE); }

inline bool asBool () const { return i==QV_TRUE; }
inline double asNum () const { return d; }
template<class T = int> inline T asInt () const { return static_cast<T>(i&~QV_TAGMASK); }
template<class T> inline T* asObject () const { return static_cast<T*>(reinterpret_cast<QObject*>(static_cast<uintptr_t>( i&~QV_TAGMASK ))); }
template<class T> inline T* asPointer () const { return reinterpret_cast<T*>(static_cast<uintptr_t>( i&~QV_TAGMASK )); }
inline QNativeFunction asNativeFunction () const { return reinterpret_cast<QNativeFunction>(asPointer<void>()); }
bool isInstanceOf (QClass* tp) const;

std::string asString () const;
const char* asCString () const;
const Swan::Range& asRange () const;

QClass& getClass (QVM& vm);
Swan::Handle asHandle ();
inline void gcVisit () { if (isObject()) asObject<QObject>()->gcVisit(); }

static const QV Null, UNDEFINED, FALSE, TRUE;
};

#endif

