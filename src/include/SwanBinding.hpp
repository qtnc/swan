	#ifndef _____SWAN_BINDING_HPP_____
#define _____SWAN_BINDING_HPP_____
#include "Swan.hpp"

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

template<class T> struct UserObjectTrait {
static inline constexpr size_t getMemSize () { return sizeof(T); }
static inline T* getPointer (void* ptr) { return static_cast<T*>(ptr); }
static inline void store (void* ptr, const T& value) { new(ptr) T(value); }
static inline void store (void* ptr, const T* value) { new(ptr) T(*value); }
static inline void storeMove (void* ptr, T&& value) { new(ptr) T(value); }
template<class... A> static inline void emplace (void* ptr, A&&... args) { new(ptr) T(args...); }
static inline void destruct (void* ptr) { getPointer(ptr)->~T(); }
};

/** Use this macro before registering a new type to make it be stored by reference */
#define SWAN_REGISTER_BYREFOBJ(T) \
namespace Swan { namespace Binding { \
template<> struct UserObjectTrait<T>  { \
static inline constexpr size_t getMemSize () { return sizeof(T*); } \
static inline T*& getPointer (void* ptr) { return *static_cast<T**>(ptr); } \
static inline void store (void* ptr, const T& value) {  getPointer(ptr) = new T(value); } \
static inline void store (void* ptr, const T* value) {  getPointer(ptr) = new T(*value); } \
static inline void storeMove (void* ptr, T&& value) {  getPointer(ptr) = new T(value); } \
template<class... A> static inline void emplace (void* ptr, A&&... args) {  getPointer(ptr) = new T(args...); } \
static inline void destruct (void* ptr) { delete getPointer(ptr); } \
}; }}

/** Use this macro before registering a new type to make it be stored using std::shared_ptr. */
#define SWAN_REGISTER_SHAREDPTROBJ(T) \
namespace Swan { namespace Binding { \
template<> struct UserObjectTrait<T>  { \
static inline constexpr size_t getMemSize () { return sizeof(std::shared_ptr<T>); } \
static inline std::shared_ptr<T>& getSharedPointer (void* ptr) { return *static_cast<std::shared_ptr<T>*>(ptr); } \
static inline T* getPointer (void* ptr) { return getSharedPointer(ptr) .get(); } \
static inline void store (void* ptr, const T& value) {  getSharedPointer(ptr) = const_cast<T&>(value) .shared_from_this(); } \
static inline void store (void* ptr, const T* value) {  getSharedPointer(ptr) = value? const_cast<T*>(value) ->shared_from_this() :nullptr; } \
static inline void storeMove (void* ptr, T&& value) {  getSharedPointer(ptr) = value.shared_from_this(); } \
template<class... A> static inline void emplace (void* ptr, A&&... args) {  getSharedPointer(ptr) = std::make_shared<T>(args...); } \
static inline void destruct (void* ptr) { getSharedPointer(ptr) .reset(); } \
}; }}

template<class T, class B = void> struct SwanGetSlot: std::false_type {};

template <class T> struct SwanGetSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
typedef T* returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isUserObject<T>(idx); }
static inline T* get (Swan::Fiber& f, int idx) {
if (f.isNullOrUndefined(idx)) return nullptr;
else return UserObjectTrait<T>::getPointer( f.getUserPointer(idx) );
}};

template <class T> struct SwanGetSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value && !is_std_function<T>::value && !is_variant<T>::value>::type> {
typedef T& returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isUserObject<T>(idx); }
static inline T& get (Swan::Fiber& f, int idx) {
return *UserObjectTrait<T>::getPointer( f.getUserPointer(idx) );
}};

template <class T> struct SwanGetSlot<T, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value && !is_std_function<T>::value  && !is_variant<T>::value>::type> {
typedef T& returnType;
static inline bool check (Swan::Fiber& f, int idx) { return f.isUserObject<T>(idx); }
static inline T& get (Swan::Fiber& f, int idx) {
return *UserObjectTrait<T>::getPointer( f.getUserPointer(idx) );
}};

template<> struct SwanGetSlot<Swan::Fiber&> {
typedef Swan::Fiber& returnType;
static Swan::Fiber& get (Swan::Fiber& f, int unused) { return f; }
static inline bool check (Swan::Fiber& f, int unused) { return true; }
};

template<> struct SwanGetSlot<Swan::Handle> {
typedef Swan::Handle returnType;
static Swan::Handle get (Swan::Fiber& f, int idx) { return f.getHandle(idx); }
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
static inline bool check (Swan::Fiber& f, int idx) { return f.isNullOrUndefined(idx); }
static inline std::nullptr_t get (Swan::Fiber& f, int idx) {  return nullptr; }
};

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

template<class T> struct SwanSetSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value  && !is_std_function<T>::value && !is_variant<T>::value>::type> {
static inline void set (Swan::Fiber& f, int idx, const T& value) { 
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
UserObjectTrait<T>::store(ptr, value);
}};

template<class T> struct SwanSetSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
static inline void set (Swan::Fiber& f, int idx, const T*  value) { 
if (!value) f.setNull(idx);
else {
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
UserObjectTrait<T>::store(ptr, value);
}}};

template<class T> struct SwanSetSlot<T, typename std::enable_if< std::is_class<T>::value  && !is_optional<T>::value  && !is_std_function<T>::value && !is_variant<T>::value>::type> {
static inline void set (Swan::Fiber& f, int idx, T& value) { 
void* ptr = f.setNewUserPointer(idx, typeid(T).hash_code());
UserObjectTrait<T>::storeMove(ptr, std::move(value));
}};

template<class T> struct SwanSetSlot<T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_enum<T>::value>::type> {
static inline void set (Swan::Fiber& f, int idx, T value) { 
f.setNum(idx, value);
}};

template<> struct SwanSetSlot<const Swan::Handle&> {
static inline void set (Swan::Fiber& f, int idx, const Swan::Handle& value) {
f.setHandle(idx, value);
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

template<class R, class... A> struct SwanSetSlot<std::function<R(A...)>> {
static inline void set (Swan::Fiber& f, int idx, const std::function<R(A...)>& func) { 
f.setCallback<R, A...>(idx, func);
}};

template<class... A> struct SwanSetSlot<std::function<void(A...)>> {
static inline void set (Swan::Fiber& f, int idx, const std::function<void(A...)>& func) { 
f.setCallback<A...>(idx, func);
}};

template<class R, class... A> struct SwanSetSlot<const std::function<R(A...)>&> {
static inline void set (Swan::Fiber& f, int idx, const std::function<R(A...)>& func) { 
f.setCallback<R, A...>(idx, func);
}};

template<class... A> struct SwanSetSlot<const std::function<void(A...)>&> {
static inline void set (Swan::Fiber& f, int idx, const std::function<void(A...)>& func) { 
f.setCallback<A...>(idx, func);
}};


template<class T, class B = void> struct SwanPushSlot: std::false_type {};

template<class T> struct SwanPushSlot<T&, typename std::enable_if< std::is_class<T>::value && !is_optional<T>::value  && !is_std_function<T>::value && !is_variant<T>::value>::type> {
static inline void push (Swan::Fiber& f, const T& value) { 
void* ptr = f.pushNewUserPointer(typeid(T).hash_code());
UserObjectTrait<T>::store(ptr, value);
}};

template<class T> struct SwanPushSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
static inline void push (Swan::Fiber& f, const T*  value) { 
if (!value) f.pushNull();
else {
void* ptr = f.pushNewUserPointer(typeid(T).hash_code());
UserObjectTrait<T>::store(ptr, value);
}}};

template<class T> struct SwanPushSlot<T, typename std::enable_if< std::is_class<T>::value  && !is_optional<T>::value  && !is_std_function<T>::value && !is_variant<T>::value>::type> {
static inline void push (Swan::Fiber& f, T& value) { 
void* ptr = f.pushNewUserPointer(typeid(T).hash_code());
UserObjectTrait<T>::storeMove(ptr, std::move(value));
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

template<> struct SwanPushSlot<const Swan::Handle&> {
static inline void push (Swan::Fiber& f, const Swan::Handle& value) {
f.pushHandle(value);
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

template<class R, class... A> struct SwanPushSlot<std::function<R(A...)>> {
static inline void push (Swan::Fiber& f, const std::function<R(A...)>& func) { 
f.pushCallback<R, A...>(func);
}};

template<class... A> struct SwanPushSlot<std::function<void(A...)>> {
static inline void push (Swan::Fiber& f, const std::function<void(A...)>& func) { 
f.pushCallback<A...>(func);
}};

template<class R, class... A> struct SwanPushSlot<const std::function<R(A...)>&> {
static inline void push (Swan::Fiber& f, const std::function<R(A...)>& func) { 
f.pushCallback<R, A...>(func);
}};

template<class... A> struct SwanPushSlot<const std::function<void(A...)>&> {
static inline void push (Swan::Fiber& f, const std::function<void(A...)>& func) { 
f.pushCallback<A...>(func);
}};

static inline void pushMultiple (Swan::Fiber& f) {}

template<class T, class... A> static inline void pushMultiple (Swan::Fiber& f, T&& arg, A&&... args) {
SwanPushSlot<T>::push(f, arg);
pushMultiple(f, args...);
}

template<class R, class... A> struct SwanGetSlot<std::function<R(A...)>> {
typedef std::function<R(A...)> returnType;
static inline returnType get (Swan::Fiber& f, int idx) { 
if (f.isNullOrUndefined(idx)) return nullptr;
Swan::VM& vm = f.getVM();
Swan::Handle handle = f.getHandle(idx);
return [=,&vm](A&&... args)->R{
Swan::Fiber& fb = vm.getActiveFiber();
Swan::ScopeLocker<Swan::VM> lock(vm);
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
if (f.isNullOrUndefined(idx)) return nullptr;
Swan::Handle handle = f.getHandle(idx);
Swan::VM& vm = f.getVM();
return [=,&vm](A&&... args){
Swan::Fiber& fb = vm.getActiveFiber();
Swan::ScopeLocker<Swan::VM> lock(vm);
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

template<class T, class R, class... A> struct SwanWrapper< R (T::*)(A...)const> {
typedef R(T::*Func)(A...)const;
template<int... S> static R callNative (sequence<S...> unused, T* obj, Func func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  return (obj->*func)( std::get<S>(params)... );  }
template<Func func> static void wrapper (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
T* obj = SwanGetSlot<T*>::get(f, 0);
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<1, A...>::extract(seq, f);
R result = callNative(seq, obj, func, params);
SwanSetSlot<R>::set(f, 0, result);
}
};

template<class T, class... A> struct SwanWrapper< void(T::*)(A...)const> {
typedef void(T::*Func)(A...)const;
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

//Begin added
#ifdef __WIN32
template<class R, class... A> struct SwanWrapper< R __stdcall(A...) > {
typedef R(*__stdcall Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, Func func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  return func( std::get<S>(params)... );  }
template<Func func> static void wrapper (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<0, A...>::extract(seq, f);
R result = callNative(seq, func, params);
SwanSetSlot<R>::set(f, 0, result);
}
};

template<class... A> struct SwanWrapper< void __stdcall(A...) > {
typedef void(*__stdcall Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, Func func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  func( std::get<S>(params)... );  }
template<Func func> static void wrapper (Swan::Fiber& f) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<0, A...>::extract(seq, f);
callNative(seq, func, params);
}
};
#endif

template<class T> struct SwanCallbackWrapper: std::false_type { };

template<class R, class... A> struct SwanCallbackWrapper< std::function<R(A...)> > {
typedef std::function<R(A...)> Func;
template<int... S> static R callNative (sequence<S...> unused, const Func& func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  return func( std::get<S>(params)... );  }
static std::function<void(Swan::Fiber&)> wrap (const Func& func) {
return [=](Swan::Fiber& f){
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<0, A...>::extract(seq, f);
R result = callNative(seq, func, params);
SwanSetSlot<R>::set(f, 0, result);
};}
};

template<class... A> struct SwanCallbackWrapper< std::function<void(A...)> > {
typedef std::function<void(A...)> Func;
template<int... S> static void callNative (sequence<S...> unused, const Func& func, const std::tuple<typename SwanGetSlot<A>::returnType...>& params) {  func( std::get<S>(params)... );  }
static std::function<void(Swan::Fiber&)> wrap (const Func& func) {
return [=](Swan::Fiber& f){
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename SwanGetSlot<A>::returnType...> params = SwanParamExtractor<0, A...>::extract(seq, f);
callNative(seq, func, params);
};}
};
//End added

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
T* ptr = SwanGetSlot<T*>::get(f, 0);
callConstructor(seq, ptr, params);
}};

template<class T> struct SwanDestructorWrapper {
static void destructor (void* userData) {
UserObjectTrait<T>::destruct(userData);
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

template<class T, class P, size_t OFFSET> struct SwanPropertyByOffsetWrapper {
static void getter (Swan::Fiber& f) {
T* obj = SwanGetSlot<T*>::get(f, 0);
P* value = reinterpret_cast<P*>(reinterpret_cast<char*>(obj) + OFFSET);
SwanSetSlot<P>::set(f, 0, *value);
}
static void setter (Swan::Fiber& f) {
T* obj = SwanGetSlot<T*>::get(f, 0);
P value = SwanGetSlot<P>::get(f, 1);
P* ptr = reinterpret_cast<P*>(reinterpret_cast<char*>(obj) + OFFSET);
*ptr = value;
SwanSetSlot<P>::set(f, 0, value);
}
};

template<class T> struct SwanValueWrapper  {
template<T* value> static void getter (Swan::Fiber& f) {
SwanSetSlot<const T&>::set(f, 0, *value);
}
template<T* value, int idx> static void setter (Swan::Fiber& f) {
*value = SwanGetSlot<T>::get(f, idx);
SwanSetSlot<const T&>::set(f, 0, *value);
}
};


} // namespace Binding

template<class T> inline T& Fiber::getUserObject (int stackIndex) {
return *Swan::Binding::UserObjectTrait<T>::getPointer(getUserPointer(stackIndex));
}

template<class T> inline T* Fiber::getOptionalUserPointer (int stackIndex, T* defaultValue) {
return Swan::Binding::UserObjectTrait<T>::getPointer(getOptionalUserPointer(stackIndex, typeid(T).hash_code(), defaultValue));
}

template<class T> inline T* Fiber::getOptionalUserPointer (int stackIndex, const std::string& key, T* defaultValue) {
return Swan::Binding::UserObjectTrait<T>::getPointer(getOptionalUserPointer(stackIndex, key, typeid(T).hash_code(), defaultValue));
}

template<class T> inline void Fiber::setUserObject (int stackIndex, const T& obj) {
void* ptr = setNewUserPointer(stackIndex, typeid(T).hash_code());
Swan::Binding::UserObjectTrait<T>::store(ptr, obj);
}

template<class T, class... A> inline void Fiber::setEmplaceUserObject (int stackIndex, A&&... args) {
void* ptr = setNewUserPointer(stackIndex, typeid(T).hash_code());
Swan::Binding::UserObjectTrait<T>::emplace(ptr, args...);
}

template<class T, class... A> inline void Fiber::pushEmplaceUserObject (A&&... args) {
void* ptr = pushNewUserPointer(typeid(T).hash_code());
Swan::Binding::UserObjectTrait<T>::emplace(ptr, args...);
}

template<class T> inline void Fiber::pushNewClass (const std::string& name, int nParents) { 
pushNewForeignClass(name, typeid(T).hash_code(), Swan::Binding::UserObjectTrait<T>::getMemSize(), nParents); 
}

template<class R, class... A> inline std::function<R(A...)> Fiber::getCallback (int idx) {
return Swan::Binding::SwanGetSlot<std::function<R(A...)>>::get(*this, idx);
}

template<class... A> inline std::function<void(A...)> Fiber::getCallback (int idx) {
return Swan::Binding::SwanGetSlot<std::function<void(A...)>>::get(*this, idx);
}

template <class R, class... A> void Fiber::setCallback (int idx, const std::function<R(A...)>& cb) {
std::function<void(Swan::Fiber&)> func = Swan::Binding::SwanCallbackWrapper<std::function<R(A...)>>::wrap(cb);
setStdFunction(idx, func);
}

template <class... A> void Fiber::setCallback (int idx, const std::function<void(A...)>& cb) {
std::function<void(Swan::Fiber&)> func = Swan::Binding::SwanCallbackWrapper<std::function<void(A...)>>::wrap(cb);
setStdFunction(idx, func);
}

template <class R, class... A> void Fiber::pushCallback (const std::function<R(A...)>& cb) {
std::function<void(Swan::Fiber&)> func = Swan::Binding::SwanCallbackWrapper<std::function<R(A...)>>::wrap(cb);
pushStdFunction(func);
}

template <class... A> void Fiber::pushCallback (const std::function<void(A...)>& cb) {
std::function<void(Swan::Fiber&)> func = Swan::Binding::SwanCallbackWrapper<std::function<void(A...)>>::wrap(cb);
pushStdFunction(func);
}

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
#define STATIC_PROPERTY(CLS,PROP) (&Swan::Binding::SwanValueWrapper<decltype(CLS::PROP)>::getter<&CLS::PROP>), (&Swan::Binding::SwanValueWrapper<decltype(CLS::PROP)>::setter<&CLS::PROP,1>)
#define GETTER(CLS,PROP) (&Swan::Binding::SwanPropertyWrapper<decltype(&CLS::PROP)>::getter<&CLS::PROP>)
#define SETTER(CLS,PROP) (&Swan::Binding::SwanPropertyWrapper<decltype(&CLS::PROP)>::setter<&CLS::PROP>)
#define MEMBER(CLS,MEM) (&Swan::Binding::SwanPropertyByOffsetWrapper<CLS, decltype(reinterpret_cast<CLS*>(0)->MEM), offsetof(CLS, MEM)>::getter), (&Swan::Binding::SwanPropertyByOffsetWrapper<CLS, decltype(reinterpret_cast<CLS*>(0)->MEM), offsetof(CLS, MEM)>::setter)

#endif
