#ifndef ___VM_ARRAY_HPP_____
#define ___VM_ARRAY_HPP_____
#include<vector>
#include<string>
#include<algorithm>
#include<cstring>
#include<iosfwd>

template <class T, class A = std::allocator<char>> 
class simple_array {
private:
char* ptr;

inline A& allocator () { return *reinterpret_cast<A*>(ptr -sizeof(size_t) -sizeof(A)); }
inline const A& allocator () const { return *reinterpret_cast<A*>(ptr -sizeof(size_t) -sizeof(A)); }
inline size_t& rfsize () { return (reinterpret_cast<size_t*>(ptr))[-1]; }
inline size_t reqsize (size_t n) { return sizeof(size_t) + sizeof(A) + n * sizeof(T); }

inline void doalloc (size_t n, const A& a) {
ptr = const_cast<A&>(a).allocate(reqsize(n)) +sizeof(size_t) +sizeof(A);
rfsize() = n;
new(&allocator()) A(a);
}

inline void dealloc () {
if (!ptr) return;
A a = allocator();
allocator().~A();
a.deallocate(ptr -sizeof(size_t) -sizeof(A), reqsize(rfsize()));
ptr = nullptr;
}

public:
inline simple_array (): ptr(nullptr) {}

inline simple_array (int n, const A& a = A()) { doalloc(n,a); }
~simple_array() {  dealloc();  }

inline simple_array (const A& a) { doalloc(0,a); }

template<class I> inline simple_array (const T& val, int n, const A& a = A()):
simple_array(n, a)
{ std::fill(begin(), end(), val); }

template<class I> inline simple_array (const I& start, const I& finish, const A& a = A()):
simple_array(finish-start, a)
{ std::copy(start, finish, begin()); }

simple_array (const simple_array& a):
simple_array(a.begin(), a.end(), a.allocator()) {}

simple_array (simple_array&& a):
ptr(a.ptr) { a.dealloc(); }

simple_array<T>& operator= (const simple_array& a) {
dealloc();
doalloc(a.size(), a.allocator());
std::copy(a.begin(), a.end(), begin());
return *this;
}

inline simple_array<T>& operator= (simple_array&& a) {
dealloc();
ptr = a.ptr;
a.ptr = nullptr;
return *this;
}

inline size_t size () const { return (reinterpret_cast<size_t*>(ptr))[-1]; }
inline bool empty () const { return size()==0; }

inline T* begin () { return reinterpret_cast<T*>(ptr); }
inline T* end () { return begin() + size(); }
inline const T* begin () const { return reinterpret_cast<T*>(ptr); }
inline const T* cbegin () const { return reinterpret_cast<T*>(ptr); }
inline const T* end () const { return begin() + size(); }
inline const T* cend () const { return begin() + size(); }
inline const T* data () const { return ptr; }

inline T& operator[] (int n) { return *(begin() +n); }
inline const T& operator[] (int n) const { return *(begin() +n); }

template <class I> void reset (const I& start, const I& finish) {
A a = allocator();
dealloc();
doalloc(finish-start, a);
std::copy(start, finish, begin());
}

void reset (size_t n) {
A a = allocator();
dealloc();
doalloc(n, a);
}

};

class simple_string: public simple_array<char> {
public:
inline simple_string (): simple_array() {}
inline simple_string (const char* s): simple_array(s? strlen(s)+1:0) { std::copy(s, s+size()+1, begin()); }
inline simple_string (const std::string& s): simple_array(s.size()+1) { std::copy(s.begin(), s.end(), begin()); }
inline size_t length () const { return size(); }
inline const char* c_str () const { return data(); }
inline std::string str () const { return data(); }
};

std::ostream& operator<< (std::ostream& out, const simple_string& s);

#endif
