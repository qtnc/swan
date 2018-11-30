	#ifndef ___QS_BINDING_HPP_1
#define ___QS_BINDING_HPP_1
#include "QScript.hpp"
#include "cpprintf.hpp"
//#include<boost/optional.hpp> 
#include<optional>
using std::optional;
//using boost::optional;

namespace QS {
namespace Binding {

template<class T> struct is_optional: std::false_type {};
template<class T> struct is_optional<optional<T>>: std::true_type {};
template <class T> struct is_std_function: std::false_type {};
template<class R, class... A> struct is_std_function<std::function<R(A...)>>: std::true_type {};
template<class R, class... A> struct is_std_function<const std::function<R(A...)>&>: std::true_type {};
template<class R, class... A> struct is_std_function<const std::function<R(A...)>>: std::true_type {};

template<class T, class B = void> struct QSGetSlot: std::false_type {};

template <class T> struct QSGetSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
typedef T* returnType;
static inline T* get (QS::Fiber& f, int idx) {
return static_cast<T*>( f.getUserPointer(idx) );
}};

template <class T> struct QSGetSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value && !is_std_function<T>::value>::type> {
typedef T& returnType;
static inline T& get (QS::Fiber& f, int idx) {
return *static_cast<T*>( f.getUserPointer(idx) );
}};

template <class T> struct QSGetSlot<T, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value && !is_std_function<T>::value>::type> {
typedef T& returnType;
static inline T& get (QS::Fiber& f, int idx) {
return *static_cast<T*>( f.getUserPointer(idx) );
}};

template<> struct QSGetSlot<QS::Fiber&> {
typedef QS::Fiber& returnType;
static QS::Fiber& get (QS::Fiber& f, int unused) { return f; }
};

template<class T> struct QSGetSlot<T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
typedef T returnType;
static inline T get (QS::Fiber& f, int idx) { 
return static_cast<T>(f.getNum(idx));
}};

template<> struct QSGetSlot<bool> {
typedef bool returnType;
static inline bool get (QS::Fiber& f, int idx) { 
return f.getBool(idx);
}};

template<> struct QSGetSlot<std::nullptr_t> {
typedef std::nullptr_t returnType;
static inline std::nullptr_t get (QS::Fiber& f, int idx) { 
return nullptr;
}};

template<> struct QSGetSlot<std::string> {
typedef std::string returnType;
static inline std::string get (QS::Fiber& f, int idx) {
return f.getString(idx);
}};

template<> struct QSGetSlot<const std::string&> {
typedef std::string returnType;
static inline std::string get (QS::Fiber& f, int idx) {
return f.getString(idx);
}};

template<> struct QSGetSlot<const char*> {
typedef const char* returnType;
static inline const char* get (QS::Fiber& f, int idx) { 
return f.getCString(idx);
}};

template<> struct QSGetSlot<QS::Range> {
typedef QS::Range& returnType;
static inline const QS::Range& get (QS::Fiber& f, int idx) { 
return f.getRange(idx);
}};

template<class T> struct QSGetSlot<optional<T>> {
typedef optional<T> returnType;
static inline optional<T> get (QS::Fiber& f, int idx) { 
optional<T> re;
if (f.getArgCount()>idx) re = QSGetSlot<T>::get(f, idx);
return re;
}};

template<class T, class B = void> struct QSSetSlot: std::false_type {};

template<class T> struct QSSetSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value>::type> {
static inline void set (QS::Fiber& f, int idx, const T& value) { 
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
new(ptr) T(value);
}};

template<class T> struct QSSetSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
static inline void set (QS::Fiber& f, int idx, const T*  value) { 
if (!value) f.setNull(idx);
else {
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
new(ptr) T(*value);
}}};

template<class T> struct QSSetSlot<T, typename std::enable_if< std::is_class<T>::value  && !is_optional<T>::value>::type> {
static inline void set (QS::Fiber& f, int idx, T& value) { 
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
new(ptr) T(std::move(value));
}};

template<class T> struct QSSetSlot<T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
static inline void set (QS::Fiber& f, int idx, T value) { 
f.setNum(idx, value);
}};

template<> struct QSSetSlot<bool> {
static inline void set (QS::Fiber& f, int idx, bool value) { 
f.setBool(idx, value);
}};

template<> struct QSSetSlot<const char*> {
static inline void set (QS::Fiber& f, int idx, const char* value) { 
f.setCString(idx, value);
}};

template<> struct QSSetSlot<char*> {
static inline void set (QS::Fiber& f, int idx, const char* value) { 
f.setCString(idx, value);
}};

template<> struct QSSetSlot<const std::string&> {
static inline void  set (QS::Fiber& f, int idx, const std::string& value) { 
f.setString(idx, value);
}};

template<> struct QSSetSlot<std::string> {
static inline void set (QS::Fiber& f, int idx, const std::string& value) { 
f.setString(idx, value);
}};

template<> struct QSSetSlot<const QS::Range&> {
static inline void  set (QS::Fiber& f, int idx, const QS::Range& value) { 
f.setRange(idx, value);
}};

template<> struct QSSetSlot<QS::Range> {
static inline void set (QS::Fiber& f, int idx, const QS::Range& value) { 
f.setRange(idx, value);
}};

template<class T> struct QSSetSlot<optional<T>> {
static inline void set (QS::Fiber& f, int idx, const optional<T>& value) { 
if (value) QSSetSlot<T>::set(f, idx, *value);
else f.setNull(idx);
}};

template<class T, class B = void> struct QSPushSlot: std::false_type {};

template<class T> struct QSPushSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value>::type> {
static inline void push (QS::Fiber& f, const T& value) { 
void* ptr = f.pushNewUserPointer(typeid(T).hash_code());
new(ptr) T(value);
}};

template<class T> struct QSPushSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
static inline void push (QS::Fiber& f, const T*  value) { 
if (!value) f.pushNull();
else {
void* ptr = f.pushNewUserPointer(typeid(T).hash_code());
new(ptr) T(*value);
}}};

template<class T> struct QSPushSlot<T, typename std::enable_if< std::is_class<T>::value  && !is_optional<T>::value>::type> {
static inline void push (QS::Fiber& f, T& value) { 
void* ptr = f.pushNewUserPointer(typeid(T).hash_code());
new(ptr) T(std::move(value));
}};

template<class T> struct QSPushSlot<T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
static inline void push (QS::Fiber& f, T value) { 
f.pushNum(value);
}};

template<class T> struct QSPushSlot<T&, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
static inline void push (QS::Fiber& f, T& value) { 
f.pushNum(value);
}};

template<> struct QSPushSlot<bool> {
static inline void push (QS::Fiber& f, bool value) { 
f.pushBool(value);
}};

template<> struct QSPushSlot<bool&> {
static inline void push (QS::Fiber& f, bool& value) { 
f.pushBool(value);
}};

template<> struct QSPushSlot<const char*> {
static inline void push (QS::Fiber& f, const char* value) { 
f.pushCString(value);
}};

template<> struct QSPushSlot<char*> {
static inline void push (QS::Fiber& f, const char* value) { 
f.pushCString(value);
}};

template<> struct QSPushSlot<const std::string&> {
static inline void  set (QS::Fiber& f, const std::string& value) { 
f.pushString(value);
}};

template<> struct QSPushSlot<std::string&> {
static inline void push (QS::Fiber& f, const std::string& value) { 
f.pushString(value);
}};

template<> struct QSPushSlot<std::string> {
static inline void push (QS::Fiber& f, const std::string& value) { 
f.pushString(value);
}};

template<> struct QSPushSlot<const QS::Range&> {
static inline void  set (QS::Fiber& f, const QS::Range& value) { 
f.pushRange(value);
}};

template<> struct QSPushSlot<QS::Range> {
static inline void push (QS::Fiber& f, const QS::Range& value) { 
f.pushRange(value);
}};

static inline void pushMultiple (QS::Fiber& f) {}

template<class T, class... A> static inline void pushMultiple (QS::Fiber& f, T&& arg, A&&... args) {
QSPushSlot<T>::push(f, arg);
pushMultiple(f, args...);
}

template<class R, class... A> struct QSGetSlot<std::function<R(A...)>> {
typedef std::function<R(A...)> returnType;
static inline returnType get (QS::Fiber& f, int idx) { 
if (f.isNull(idx)) return nullptr;
QS::Handle handle = f.getHandle(idx);
return [=](A&&... args)->R{
QS::Fiber& fb = QS::VM::getVM().getActiveFiber();
QS::ScopeLocker<QS::Fiber> lock(fb);
fb.pushHandle(handle);
pushMultiple(fb, args...);
fb.call(sizeof...(A));
R result = QSGetSlot<R>::get(fb, -1);
fb.pop();
return result;
};
}};

template<class... A> struct QSGetSlot<std::function<void(A...)>> {
typedef std::function<void(A...)> returnType;
static inline returnType get (QS::Fiber& f, int idx) { 
if (f.isNull(idx)) return nullptr;
QS::Handle handle = f.getHandle(idx);
return [=](A&&... args){
QS::Fiber& fb = QS::VM::getVM().getActiveFiber();
QS::ScopeLocker<QS::Fiber> lock(fb);
fb.pushHandle(handle);
pushMultiple(fb, args...);
fb.call(sizeof...(A));
fb.pop();
};
}};

template<class R, class... A> struct QSGetSlot<const std::function<R(A...)>&> {
typedef std::function<R(A...)> returnType;
static inline returnType get (QS::Fiber& f, int idx) { 
return QSGetSlot<returnType>::get(f, idx);
}};

template<int ...> struct sequence {};
template<int N, int ...S> struct sequence_generator: sequence_generator<N-1, N-1, S...> {};
template<int ...S> struct sequence_generator<0, S...>{ typedef sequence<S...> type; };

template<int START, class... A> struct QSParamExtractor {
template<int N, typename... Ts> using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;
template<int... S> static inline std::tuple<typename QSGetSlot<A>::returnType...> extract (sequence<S...> unused, QS::Fiber& f) {  return std::forward_as_tuple( extract1<S, NthTypeOf<S,A...>>(f)... );  }
template<int S, class E> static typename QSGetSlot<E>::returnType  extract1 (QS::Fiber& f) { return QSGetSlot<E>::get(f, S+START); }
};

template <class UNUSED> struct QSWrapper: std::false_type { };

template<class T, class R, class... A> struct QSWrapper< R (T::*)(A...) > {
typedef R(T::*Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, T* obj, Func func, const std::tuple<typename QSGetSlot<A>::returnType...>& params) {  return (obj->*func)( std::get<S>(params)... );  }
template<Func func> static void wrapper (QS::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
T* obj = QSGetSlot<T*>::get(f, 0);
std::tuple<typename QSGetSlot<A>::returnType...> params = QSParamExtractor<1, A...>::extract(seq, f);
R result = callNative(seq, obj, func, params);
QSSetSlot<R>::set(f, 0, result);
}
};

template<class T, class... A> struct QSWrapper< void(T::*)(A...) > {
typedef void(T::*Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, T* obj, Func func, const std::tuple<typename QSGetSlot<A>::returnType...>& params) {  (obj->*func)( std::get<S>(params)... );  }
template<Func func> static void wrapper (QS::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
T* obj = QSGetSlot<T*>::get(f, 0);
std::tuple<typename QSGetSlot<A>::returnType...> params = QSParamExtractor<1, A...>::extract(seq, f);
callNative(seq, obj, func, params);
}
};

template<class R, class... A> struct QSWrapper< R(A...) > {
typedef R(*Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, Func func, const std::tuple<typename QSGetSlot<A>::returnType...>& params) {  return func( std::get<S>(params)... );  }
template<Func func> static void wrapper (QS::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename QSGetSlot<A>::returnType...> params = QSParamExtractor<0, A...>::extract(seq, f);
R result = callNative(seq, func, params);
QSSetSlot<R>::set(f, 0, result);
}
};

template<class... A> struct QSWrapper< void(A...) > {
typedef void(*Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, Func func, const std::tuple<typename QSGetSlot<A>::returnType...>& params) {  func( std::get<S>(params)... );  }
template<Func func> static void wrapper (QS::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename QSGetSlot<A>::returnType...> params = QSParamExtractor<0, A...>::extract(seq, f);
callNative(seq, func, params);
}
};

template <class UNUSED> struct QSStaticWrapper: std::false_type { };

template<class R, class... A> struct QSStaticWrapper< R(A...) > {
typedef R(*Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, Func func, const std::tuple<typename QSGetSlot<A>::returnType...>& params) {  return func( std::get<S>(params)... );  }
template<Func func> static void wrapper (QS::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename QSGetSlot<A>::returnType...> params = QSParamExtractor<1, A...>::extract(seq, f);
R result = callNative(seq, func, params);
QSSetSlot<R>::set(f, 0, result);
}
};

template<class... A> struct QSStaticWrapper< void(A...) > {
typedef void(*Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, Func func, const std::tuple<typename QSGetSlot<A>::returnType...>& params) {  func( std::get<S>(params)... );  }
template<Func func> static void wrapper (QS::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename QSGetSlot<A>::returnType...> params = QSParamExtractor<1, A...>::extract(seq, f);
callNative(seq, func, params);
}
};



template<class T, class... A> struct QSConstructorWrapper {
template<int... S> static void callConstructor (sequence<S...> unused, void* ptr, const std::tuple<typename QSGetSlot<A>::returnType...>& params) {  new(ptr) T( std::get<S>(params)... );  }
static void constructor (QS::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename QSGetSlot<A>::returnType...> params = QSParamExtractor<1, A...>::extract(seq, f);
void* ptr = f.getUserPointer(0);
callConstructor(seq, ptr, params);
}};

template<class T> struct QSDestructorWrapper {
static void destructor (void* userData) {
T* obj = reinterpret_cast<T*>(userData);
obj->~T();
}};

template<class UNUSED> struct QSPropertyWrapper: std::false_type {};

template<class T, class P> struct QSPropertyWrapper<P T::*> {
typedef P T::*Prop;
template<Prop prop> static void getter (QS::Fiber& f) {
T* obj = QSGetSlot<T*>::get(f, 0);
P value = (obj->*prop);
QSSetSlot<P>::set(f, 0, value);
}
template<Prop prop> static void setter (QS::Fiber& f) {
T* obj = QSGetSlot<T*>::get(f, 0);
P value = QSGetSlot<P>::get(f, 1);
(obj->*prop) = value;
QSSetSlot<P>::set(f, 0, value);
}
};

} // namespace Binding


template <class T, class... A> inline void Fiber::registerConstructor () {
registerMethod("constructor", &QS::Binding::QSConstructorWrapper<T, A...>::constructor);
}

template <class T> inline void Fiber::registerDestructor () {
storeDestructor(&QS::Binding::QSDestructorWrapper<T>::destructor);
}

} // namespace QS

#define FUNCTION(...) (&QS::Binding::QSWrapper<decltype(__VA_ARGS__)>::wrapper<&(__VA_ARGS__)>)
#define METHOD(CLS,...) (&QS::Binding::QSWrapper<decltype(&CLS::__VA_ARGS__)>::wrapper<&CLS::__VA_ARGS__>)
#define STATIC_METHOD(...) (&QS::Binding::QSStaticWrapper<decltype(__VA_ARGS__)>::wrapper<&(__VA_ARGS__)>)
#define PROPERTY(CLS,PROP) (&QS::Binding::QSPropertyWrapper<decltype(&CLS::PROP)>::getter<&CLS::PROP>), (&QS::Binding::QSPropertyWrapper<decltype(&CLS::PROP)>::setter<&CLS::PROP>)
#define GETTER(CLS,PROP) (&QS::Binding::QSPropertyWrapper<decltype(&CLS::PROP)>::getter<&CLS::PROP>)
#define SETTER(CLS,PROP) (&QS::Binding::QSPropertyWrapper<decltype(&CLS::PROP)>::setter<&CLS::PROP>)

#endif
