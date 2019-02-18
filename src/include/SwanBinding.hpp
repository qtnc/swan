	#ifndef _____SWAN_BINDING_HPP_____
#define _____SWAN_BINDING_HPP_____
#include "Swan.hpp"
#include "cpprintf.hpp"

namespace Swan {
namespace Binding {

template<class T> struct is_optional: std::false_type {};

#ifdef __cpp_lib_optional
using std::optional;
template<class T> struct is_optional<optional<T>>: std::true_type {};
#endif

template <class T> struct is_std_function: std::false_type {};
template<class R, class... A> struct is_std_function<std::function<R(A...)>>: std::true_type {};
template<class R, class... A> struct is_std_function<const std::function<R(A...)>&>: std::true_type {};
template<class R, class... A> struct is_std_function<const std::function<R(A...)>>: std::true_type {};

template<class T> struct is_variant: std::false_type {};

#ifdef __cpp_lib_variant
template<class... A> struct is_variant<std::variant<A...>>: std::true_type {};
template<class... A> struct is_variant<const std::variant<A...>>: std::true_type {};
template<class... A> struct is_variant<const std::variant<A...>&>: std::true_type {};
#endif

template<int ...> struct sequence {};
template<int N, int ...S> struct sequence_generator: sequence_generator<N-1, N-1, S...> {};
template<int ...S> struct sequence_generator<0, S...>{ typedef sequence<S...> type; };

template<class T, class B = void> struct SwanGetSlot: std::false_type {};

template <class T> struct SwanGetSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
typedef T* returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isUserObject<T>(idx); }
static inline T* get (Swan::Fiber& f, int idx) {
return static_cast<T*>( f.getUserPointer(idx) );
}};

template <class T> struct SwanGetSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value && !is_std_function<T>::value && !is_variant<T>::value>::type> {
typedef T& returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isUserObject<T>(idx); }
static inline T& get (Swan::Fiber& f, int idx) {
return *static_cast<T*>( f.getUserPointer(idx) );
}};

template <class T> struct SwanGetSlot<T, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value && !is_std_function<T>::value  && !is_variant<T>::value>::type> {
typedef T& returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isUserObject<T>(idx); }
static inline T& get (Swan::Fiber& f, int idx) {
return *static_cast<T*>( f.getUserPointer(idx) );
}};

template<> struct SwanGetSlot<Swan::Fiber&> {
typedef Swan::Fiber& returnType;
static Swan::Fiber& get (Swan::Fiber& f, int unused) { return f; }
static inline bool check (Swan::Fiber& f, int unused) { return true; }
};

template<class T> struct SwanGetSlot<T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
typedef T returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isNum(idx); }
static inline T get (Swan::Fiber& f, int idx) { 
return static_cast<T>(f.getNum(idx));
}};

template<> struct SwanGetSlot<bool> {
typedef bool returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isBool(idx); }
static inline bool get (Swan::Fiber& f, int idx) { 
return f.getBool(idx);
}};

template<> struct SwanGetSlot<std::nullptr_t> {
typedef std::nullptr_t returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isNull(idx); }
static inline std::nullptr_t get (Swan::Fiber& f, int idx) { 
return nullptr;
}};

template<> struct SwanGetSlot<std::string> {
typedef std::string returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isString(idx); }
static inline std::string get (Swan::Fiber& f, int idx) {
return f.getString(idx);
}};

template<> struct SwanGetSlot<const std::string&> {
typedef std::string returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isString(idx); }
static inline std::string get (Swan::Fiber& f, int idx) {
return f.getString(idx);
}};

template<> struct SwanGetSlot<const char*> {
typedef const char* returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isString(idx); }
static inline const char* get (Swan::Fiber& f, int idx) { 
return f.getCString(idx);
}};

template<> struct SwanGetSlot<Swan::Range> {
typedef Swan::Range& returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isRange(idx); }
static inline const Swan::Range& get (Swan::Fiber& f, int idx) { 
return f.getRange(idx);
}};

#ifdef __cpp_lib_optional
template<class T> struct SwanGetSlot<optional<T>> {
typedef optional<T> returnType;
static inline bool check (Swan::Fiber& f, int idx) { return SwanGetSlot<T>::check(f, idx); }
static inline optional<T> get (Swan::Fiber& f, int idx) { 
optional<T> re;
if (f.getArgCount()>idx && check(f, idx)) re = SwanGetSlot<T>::get(f, idx);
return re;
}};
#endif

#ifdef __cpp_lib_variant
template<int Z, class V, class... A> struct SwanGetSlotVariant {
};

template<int Z, class V, class T, class... A> struct SwanGetSlotVariant<Z, V, T, A...> {
static inline void getv (Swan::Fiber& f, int idx, V& var) {
if (SwanGetSlot<T>::check(f, idx)) var = SwanGetSlot<T>::get(f, idx);
else SwanGetSlotVariant<sizeof...(A), V, A...>::getv(f, idx, var);
}};

template<class V> struct SwanGetSlotVariant<0, V>  {
static inline void getv (Swan::Fiber& f, int idx, V& var) { }
};

template<class... A> struct SwanGetSlot<std::variant<A...>> {
typedef std::variant<A...> returnType;
static inline returnType get (Swan::Fiber& f, int idx) { 
returnType re;
SwanGetSlotVariant<sizeof...(A), returnType, A...>::getv(f, idx, re);
return re;
}};

template<class... A> struct SwanGetSlot<const std::variant<A...>&> {
typedef std::variant<A...> returnType;
static inline returnType get (Swan::Fiber& f, int idx) { 
return SwanGetSlot<returnType>::get(f, idx);
}};
#endif

template<class T, class B = void> struct SwanSetSlot: std::false_type {};

template<class T> struct SwanSetSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value  && !is_variant<T>::value>::type> {
static inline void set (Swan::Fiber& f, int idx, const T& value) { 
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
new(ptr) T(value);
}};

template<class T> struct SwanSetSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
static inline void set (Swan::Fiber& f, int idx, const T*  value) { 
if (!value) f.setNull(idx);
else {
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
new(ptr) T(*value);
}}};

template<class T> struct SwanSetSlot<T, typename std::enable_if< std::is_class<T>::value  && !is_optional<T>::value  && !is_variant<T>::value>::type> {
static inline void set (Swan::Fiber& f, int idx, T& value) { 
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
new(ptr) T(std::move(value));
}};

template<class T> struct SwanSetSlot<T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
static inline void set (Swan::Fiber& f, int idx, T value) { 
f.setNum(idx, value);
}};

template<class T> struct SwanSetSlot<const T&, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
static inline void set (Swan::Fiber& f, int idx, const T& value) { 
f.setNum(idx, value);
}};

template<> struct SwanSetSlot<bool> {
static inline void set (Swan::Fiber& f, int idx, bool value) { 
f.setBool(idx, value);
}};

template<> struct SwanSetSlot<const bool&> {
static inline void set (Swan::Fiber& f, int idx, const bool& value) { 
f.setBool(idx, value);
}};

template<> struct SwanSetSlot<const char*> {
static inline void set (Swan::Fiber& f, int idx, const char* value) { 
f.setCString(idx, value);
}};

template<> struct SwanSetSlot<char*> {
static inline void set (Swan::Fiber& f, int idx, const char* value) { 
f.setCString(idx, value);
}};

template<> struct SwanSetSlot<const std::string&> {
static inline void  set (Swan::Fiber& f, int idx, const std::string& value) { 
f.setString(idx, value);
}};

template<> struct SwanSetSlot<std::string> {
static inline void set (Swan::Fiber& f, int idx, const std::string& value) { 
f.setString(idx, value);
}};

template<> struct SwanSetSlot<const Swan::Range&> {
static inline void  set (Swan::Fiber& f, int idx, const Swan::Range& value) { 
f.setRange(idx, value);
}};

template<> struct SwanSetSlot<Swan::Range> {
static inline void set (Swan::Fiber& f, int idx, const Swan::Range& value) { 
f.setRange(idx, value);
}};

#ifdef __cpp_lib_optional
template<class T> struct SwanSetSlot<optional<T>> {
static inline void set (Swan::Fiber& f, int idx, const optional<T>& value) { 
if (value) SwanSetSlot<T>::set(f, idx, *value);
else f.setNull(idx);
}};
#endif

#ifdef __cpp_lib_variant
template<class... A> struct SwanSetSlot<std::variant<A...>> {
static inline void set (Swan::Fiber& f, int idx, const std::variant<A...>& value) { 
std::visit([&](auto&& x){ SwanSetSlot<decltype(x)>::set(f, idx, x); }, value);
}};

template<class... A> struct SwanSetSlot<const std::variant<A...>&> {
static inline void set (Swan::Fiber& f, int idx, const std::variant<A...>& value) { 
std::visit([&](auto&& x){ SwanSetSlot<decltype(x)>::set(f, idx, x); }, value);
}};
#endif

template<class T, class B = void> struct SwanPushSlot: std::false_type {};

template<class T> struct SwanPushSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value  && !is_variant<T>::value>::type> {
static inline void push (Swan::Fiber& f, const T& value) { 
void* ptr = f.pushNewUserPointer(typeid(T).hash_code());
new(ptr) T(value);
}};

template<class T> struct SwanPushSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
static inline void push (Swan::Fiber& f, const T*  value) { 
if (!value) f.pushNull();
else {
void* ptr = f.pushNewUserPointer(typeid(T).hash_code());
new(ptr) T(*value);
}}};

template<class T> struct SwanPushSlot<T, typename std::enable_if< std::is_class<T>::value  && !is_optional<T>::value  && !is_variant<T>::value>::type> {
static inline void push (Swan::Fiber& f, T& value) { 
void* ptr = f.pushNewUserPointer(typeid(T).hash_code());
new(ptr) T(std::move(value));
}};

template<class T> struct SwanPushSlot<T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
static inline void push (Swan::Fiber& f, T value) { 
f.pushNum(value);
}};

template<class T> struct SwanPushSlot<const T&, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
static inline void push (Swan::Fiber& f, const T& value) { 
f.pushNum(value);
}};

template<class T> struct SwanPushSlot<T&, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
static inline void push (Swan::Fiber& f, T& value) { 
f.pushNum(value);
}};

template<> struct SwanPushSlot<bool> {
static inline void push (Swan::Fiber& f, bool value) { 
f.pushBool(value);
}};

template<> struct SwanPushSlot<const bool&> {
static inline void push (Swan::Fiber& f, const bool& value) { 
f.pushBool(value);
}};

template<> struct SwanPushSlot<bool&> {
static inline void push (Swan::Fiber& f, bool& value) { 
f.pushBool(value);
}};

template<> struct SwanPushSlot<const char*> {
static inline void push (Swan::Fiber& f, const char* value) { 
f.pushCString(value);
}};

template<> struct SwanPushSlot<char*> {
static inline void push (Swan::Fiber& f, const char* value) { 
f.pushCString(value);
}};

template<> struct SwanPushSlot<const std::string&> {
static inline void  set (Swan::Fiber& f, const std::string& value) { 
f.pushString(value);
}};

template<> struct SwanPushSlot<std::string&> {
static inline void push (Swan::Fiber& f, const std::string& value) { 
f.pushString(value);
}};

template<> struct SwanPushSlot<std::string> {
static inline void push (Swan::Fiber& f, const std::string& value) { 
f.pushString(value);
}};

template<> struct SwanPushSlot<const Swan::Range&> {
static inline void  set (Swan::Fiber& f, const Swan::Range& value) { 
f.pushRange(value);
}};

template<> struct SwanPushSlot<Swan::Range> {
static inline void push (Swan::Fiber& f, const Swan::Range& value) { 
f.pushRange(value);
}};

#ifdef __cpp_lib_variant
template<class... A> struct SwanPushSlot<std::variant<A...>> {
static inline void push (Swan::Fiber& f, const std::variant<A...>& value) { 
std::visit([&](auto&& x){ SwanPushSlot<decltype(x)>::push(f, x); }, value);
}};

template<class... A> struct SwanPushSlot<const std::variant<A...>&> {
static inline void push (Swan::Fiber& f, const std::variant<A...>& value) { 
std::visit([&](auto&& x){ SwanPushSlot<decltype(x)>::push(f, x); }, value);
}};
#endif

static inline void pushMultiple (Swan::Fiber& f) {}

template<class T, class... A> static inline void pushMultiple (Swan::Fiber& f, T&& arg, A&&... args) {
SwanPushSlot<T>::push(f, arg);
pushMultiple(f, args...);
}

template<class R, class... A> struct SwanGetSlot<std::function<R(A...)>> {
typedef std::function<R(A...)> returnType;
static inline returnType get (Swan::Fiber& f, int idx) { 
if (f.isNull(idx)) return nullptr;
Swan::Handle handle = f.getHandle(idx);
return [=](A&&... args)->R{
Swan::Fiber& fb = Swan::VM::getVM().getActiveFiber();
Swan::ScopeLocker<Swan::Fiber> lock(fb);
fb.pushHandle(handle);
pushMultiple(fb, args...);
fb.call(sizeof...(A));
R result = SwanGetSlot<R>::get(fb, -1);
fb.pop();
return result;
};
}};

template<class... A> struct SwanGetSlot<std::function<void(A...)>> {
typedef std::function<void(A...)> returnType;
static inline returnType get (Swan::Fiber& f, int idx) { 
if (f.isNull(idx)) return nullptr;
Swan::Handle handle = f.getHandle(idx);
return [=](A&&... args){
Swan::Fiber& fb = Swan::VM::getVM().getActiveFiber();
Swan::ScopeLocker<Swan::Fiber> lock(fb);
fb.pushHandle(handle);
pushMultiple(fb, args...);
fb.call(sizeof...(A));
fb.pop();
};
}};

template<class R, class... A> struct SwanGetSlot<const std::function<R(A...)>&> {
typedef std::function<R(A...)> returnType;
static inline returnType get (Swan::Fiber& f, int idx) { 
return SwanGetSlot<returnType>::get(f, idx);
}};

template<int START, class... A> struct SwanParamExtractor {
template<int N, typename... Ts> using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;
template<int... S> static inline std::tuple<typename SwanGetSlot<A>::returnType...> extract (sequence<S...> unused, Swan::Fiber& f) {  return std::forward_as_tuple( extract1<S, NthTypeOf<S,A...>>(f)... );  }
template<int S, class E> static inline typename SwanGetSlot<E>::returnType  extract1 (Swan::Fiber& f) { return SwanGetSlot<E>::get(f, S+START); }
};

template <class UNUSED> struct SwanWrapper: std::false_type { };

template<class T, class R, class... A> struct SwanWrapper< R (T::*)(A...) > {
typedef R(T::*Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, T* obj, Func func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  return (obj->*func)( std::get<S>(params)... );  }
template<Func func> static void wrapper (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
T* obj = SwanGetSlot<T*>::get(f, 0);
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<1, A...>::extract(seq, f);
R result = callNative(seq, obj, func, params);
SwanSetSlot<R>::set(f, 0, result);
}
};

template<class T, class... A> struct SwanWrapper< void(T::*)(A...) > {
typedef void(T::*Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, T* obj, Func func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  (obj->*func)( std::get<S>(params)... );  }
template<Func func> static void wrapper (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
T* obj = SwanGetSlot<T*>::get(f, 0);
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<1, A...>::extract(seq, f);
callNative(seq, obj, func, params);
}
};

template<class R, class... A> struct SwanWrapper< R(A...) > {
typedef R(*Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, Func func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  return func( std::get<S>(params)... );  }
template<Func func> static void wrapper (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<0, A...>::extract(seq, f);
R result = callNative(seq, func, params);
SwanSetSlot<R>::set(f, 0, result);
}
};

template<class... A> struct SwanWrapper< void(A...) > {
typedef void(*Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, Func func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  func( std::get<S>(params)... );  }
template<Func func> static void wrapper (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<0, A...>::extract(seq, f);
callNative(seq, func, params);
}
};

template <class UNUSED> struct SwanStaticWrapper: std::false_type { };

template<class R, class... A> struct SwanStaticWrapper< R(A...) > {
typedef R(*Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, Func func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  return func( std::get<S>(params)... );  }
template<Func func> static void wrapper (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<1, A...>::extract(seq, f);
R result = callNative(seq, func, params);
SwanSetSlot<R>::set(f, 0, result);
}
};

template<class... A> struct SwanStaticWrapper< void(A...) > {
typedef void(*Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, Func func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  func( std::get<S>(params)... );  }
template<Func func> static void wrapper (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<1, A...>::extract(seq, f);
callNative(seq, func, params);
}
};



template<class T, class... A> struct SwanConstructorWrapper {
template<int... S> static void callConstructor (sequence<S...> unused, void* ptr, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  new(ptr) T( std::get<S>(params)... );  }
static void constructor (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<1, A...>::extract(seq, f);
void* ptr = f.getUserPointer(0);
callConstructor(seq, ptr, params);
}};

template<class T> struct SwanDestructorWrapper {
static void destructor (void* userData) {
T* obj = reinterpret_cast<T*>(userData);
obj->~T();
}};

template<class UNUSED> struct SwanPropertyWrapper: std::false_type {};

template<class T, class P> struct SwanPropertyWrapper<P T::*> {
typedef P T::*Prop;
template<Prop prop> static void getter (Swan::Fiber& f) {
T* obj = SwanGetSlot<T*>::get(f, 0);
P value = (obj->*prop);
SwanSetSlot<P>::set(f, 0, value);
}
template<Prop prop> static void setter (Swan::Fiber& f) {
T* obj = SwanGetSlot<T*>::get(f, 0);
P value = SwanGetSlot<P>::get(f, 1);
(obj->*prop) = value;
SwanSetSlot<P>::set(f, 0, value);
}
};

} // namespace Binding


template <class T, class... A> inline void Fiber::registerConstructor () {
registerMethod("constructor", &Swan::Binding::SwanConstructorWrapper<T, A...>::constructor);
}

template <class T> inline void Fiber::registerDestructor () {
storeDestructor(&Swan::Binding::SwanDestructorWrapper<T>::destructor);
}

} // namespace Swan

#define FUNCTION(...) (&Swan::Binding::SwanWrapper<decltype(__VA_ARGS__)>::wrapper<&(__VA_ARGS__)>)
#define METHOD(CLS,...) (&Swan::Binding::SwanWrapper<decltype(&CLS::__VA_ARGS__)>::wrapper<&CLS::__VA_ARGS__>)
#define STATIC_METHOD(...) (&Swan::Binding::SwanStaticWrapper<decltype(__VA_ARGS__)>::wrapper<&(__VA_ARGS__)>)
#define PROPERTY(CLS,PROP) (&Swan::Binding::SwanPropertyWrapper<decltype(&CLS::PROP)>::getter<&CLS::PROP>), (&Swan::Binding::SwanPropertyWrapper<decltype(&CLS::PROP)>::setter<&CLS::PROP>)
#define GETTER(CLS,PROP) (&Swan::Binding::SwanPropertyWrapper<decltype(&CLS::PROP)>::getter<&CLS::PROP>)
#define SETTER(CLS,PROP) (&Swan::Binding::SwanPropertyWrapper<decltype(&CLS::PROP)>::setter<&CLS::PROP>)

#endif