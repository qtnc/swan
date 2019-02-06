#ifndef _____SWAN_EXECUTION_STACK_HPP_____
#define _____SWAN_EXECUTION_STACK_HPP_____
#include<functional>

template<class T, class F = std::function<void(const T*, const T*)>, class Alloc = std::allocator<T>> struct execution_stack {
T *base, *top, *finish;
Alloc allocator;
F callback;
inline execution_stack (const F& cb = nullptr, size_t initialCapacity = 4):  base(nullptr), top(nullptr), finish(nullptr), callback(cb) {
base = allocator.allocate(initialCapacity);
top = base;
finish = base+initialCapacity;
}
inline ~execution_stack () { allocator.deallocate(base, (finish-base) * sizeof(T)); }
void reserve (size_t newCapacity) {
if (newCapacity<=finish-base) return;
T* newBase = allocator.allocate(newCapacity);
memcpy(newBase, base, (top-base) * sizeof(T));
callback(base, newBase);
allocator.deallocate(base, (finish-base) * sizeof(T));
top = newBase + (top-base);
base = newBase;
finish = base + newCapacity;
}
inline void resize (size_t newSize) {
reserve(newSize);
top = base + newSize;
}
inline void push_back (const T& x) {
if (top==finish)  reserve(2 * (finish-base));
*top++ = x;
}
inline T& pop_back () {
return *top--;
}
template<class I> void insert (T* pos, I begin, const I& end) {
size_t index = pos-base, len = std::distance(begin, end);
reserve( (top-base) + len);
pos = base+index;
memmove(pos+len, pos, (top-pos) * sizeof(T));
while(begin!=end) *pos++ = *begin++;
top += len;
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

