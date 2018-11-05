#ifndef ___QS_BINDING_HPP_1
#define ___QS_BINDING_HPP_1
#include "QScript.hpp"
#include "cpprintf.hpp"
#include<boost/optional.hpp> //todo: switch to C++17 and use std::optional
using boost::optional;

namespace QS {
namespace Binding {

template<class T> struct is_optional: std::false_type {};
template<class T> struct is_optional<optional<T>>: std::true_type {};

template<class T, class B = void> struct QSGetSlot: std::false_type {};

template <class T> struct QSGetSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
typedef T* returnType;
static inline T* get (QS::Fiber& f, int idx) {
return static_cast<T*>( f.getUserPointer(idx) );
}};

template <class T> struct QSGetSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value>::type> {
typedef T& returnType;
static inline T& get (QS::Fiber& f, int idx) {
return *static_cast<T*>( f.getUserPointer(idx) );
}};

template <class T> struct QSGetSlot<T, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value>::type> {
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

template<class T> struct QSSetSlot<T&&, typename std::enable_if< std::is_class<T>::value  && !is_optional<T>::value>::type> {
static inline void set (QS::Fiber& f, int idx, T&& value) { 
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
static inline void set (QS::Fiber& f, int idx, const T& value) { 
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
new(ptr) T(value);
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

#define FUNCTION(FUNC) (&QS::Binding::QSWrapper<decltype(FUNC)>::wrapper<&FUNC>)
#define METHOD(CLS,METH) (&QS::Binding::QSWrapper<decltype(&CLS::METH)>::wrapper<&CLS::METH>)
#define STATIC_METHOD(FUNC) (&QS::Binding::QSStaticWrapper<decltype(FUNC)>::wrapper<&FUNC>)
#define PROPERTY(CLS,PROP) (&QS::Binding::QSPropertyWrapper<decltype(&CLS::PROP)>::getter<&CLS::PROP>), (&QS::Binding::QSPropertyWrapper<decltype(&CLS::PROP)>::setter<&CLS::PROP>)
#define GETTER(CLS,PROP) (&QS::Binding::QSPropertyWrapper<decltype(&CLS::PROP)>::getter<&CLS::PROP>)
#define SETTER(CLS,PROP) (&QS::Binding::QSPropertyWrapper<decltype(&CLS::PROP)>::setter<&CLS::PROP>)

#endif
