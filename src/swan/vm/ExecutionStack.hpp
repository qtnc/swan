#ifndef _____SWAN_EXECUTION_STACK_HPP_____
#define _____SWAN_EXECUTION_STACK_HPP_____
#include<functional>
#include "../../include/cpprintf.hpp"

template<class T, class Alloc = std::allocator<T>, class F = std::function<void(const T*, const T*)>> struct execution_stack {
T *base, *top, *finish;
Alloc allocator;
F callback;
inline execution_stack (const F& cb = nullptr, size_t initialCapacity = 4, const Alloc& alloc0 = std::allocator<T>()):  
base(nullptr), top(nullptr), finish(nullptr), 
callback(cb), allocator(alloc0) {
base = allocator.allocate(initialCapacity * sizeof(T));
top = base;
finish = base+initialCapacity;
}
inline ~execution_stack () { allocator.deallocate(base, (finish-base) * sizeof(T)); }
std::pair<T*,size_t> reserve (size_t newCapacity) {
auto re = std::make_pair(base, (finish-base) * sizeof(T));
if (newCapacity<=finish-base) return re;
T* newBase = allocator.allocate(newCapacity * sizeof(T));
callback(base, newBase);
memcpy(newBase, base, (top-base) * sizeof(T));
top = newBase + (top-base);
base = newBase;
finish = base + newCapacity;
return re;
}
inline void finishReserve (std::pair<T*, size_t>& old) {
if (old.first==base) return;
allocator.deallocate(old.first, old.second);
}
inline void resize (size_t newSize) {
auto old = reserve(newSize);
top = base + newSize;
finishReserve(old);
}
inline void push_back (const T& x) {
if (top!=finish)  { *top++=x; return; }
auto old = reserve(2 * (finish-base));
*top++ = x;
finishReserve(old);
}
inline T& pop_back () {
return *top--;
}
template<class I> void insert (T* pos, I begin, const I& end) {
size_t index = pos-base, len = std::distance(begin, end);
auto old = reserve( (top-base) + len);
pos = base+index;
memmove(pos+len, pos, (top-pos) * sizeof(T));
while(begin!=end) *pos++ = *begin++;
top += len;
finishReserve(old);
}
inline void erase (T* begin, T* end) { 
memmove(begin, end, (top-end)*sizeof(T));
top -= (end-begin);
}
inline void insert (T* pos, const T& val) { insert(pos, &val, &val+1); }
inline void erase (T* pos) { erase(pos, pos+1); }
inline size_t size () const { return top-base; }
inline bool empty () { return size()==0; }
inline T& at (int i) { return *(base+i); }
inline T& operator[] (int i) { return *(base+i); }
inline const T& at (int i) const { return *(base+i); }
inline const T& operator[] (int i) const { return *(base+i); }
inline T* begin () { return base; }
inline const T* begin () const { return base; }
inline T* end () { return top; }
inline const T* end () const { return top; }
inline T& back () { return *(top -1); }
inline const T& back () const { return *(top -1); }
};

#endif

