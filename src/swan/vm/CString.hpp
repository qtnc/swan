#ifndef ___CSTRING_HPP___
#define ___CSTRING_HPP___
#include<vector>
#include<string>
#include<algorithm>
#include<cstring>
#include<memory>
#include<type_traits>
#include<iosfwd>


class c_string {
private:
std::unique_ptr<char[]> ptr;

public:
template<class I> inline void assign (const I& start, const I& finish) {
size_t size = finish-start;
ptr = std::make_unique<char[]>(size+1);
std::copy(start, finish, &ptr[0]);
ptr[size] = 0;
}
inline void assign (const char* s) {
if (s) assign(s, s+strlen(s));
else ptr.reset();
}
inline c_string(): ptr(nullptr) {}
template<class I> inline c_string (const I& start, const I& finish): ptr(nullptr) { assign(start, finish); }
inline c_string (const std::string& s): ptr(nullptr) { assign(s.begin(), s.end()); }
inline c_string(const c_string& c): ptr(nullptr) { assign(c.ptr?&c.ptr[0]:nullptr); }
inline c_string(c_string&& c): ptr(std::move(c.ptr)) {}
inline c_string& operator= (const c_string& c) { assign(c.ptr?&c.ptr[0]:nullptr); return *this; }
inline c_string& operator= (c_string&& c) { ptr = std::move(c.ptr); return *this; }
inline c_string& operator= (const char* s) { assign(s); return *this; }
inline c_string& operator= (const std::string& s) { assign(s.begin(), s.end()); return *this; }
inline const char* begin () const { return &ptr[0]; }
inline const char* data () const { return ptr? &ptr[0] : nullptr; }
inline const char* c_str () const { return ptr? &ptr[0] : nullptr; }
std::string str () const { return ptr?&ptr[0]:""; }
inline size_t size () const { return ptr?strlen(&ptr[0]):0; }
inline bool empty () const { return size()==0; }
inline explicit operator bool () const { return size()>0; }
};

std::ostream& operator<< (std::ostream& out, const c_string& s);

#endif
