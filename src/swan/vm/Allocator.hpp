#ifndef ___VM_ALLOCATOR_HPP_____
#define ___VM_ALLOCATOR_HPP_____

void* vm_alloc (struct QVM& vm, size_t n);
void vm_dealloc (struct QVM& vm, void* p, size_t n);

template<class T>
struct trace_allocator {
typedef T value_type;
typedef T* pointer;
typedef T& reference;
typedef const T& const_reference;
typedef size_t size_type;
typedef ptrdiff_t diffrence_type;

struct QVM& vm;

template<class U> struct rebind { typedef trace_allocator<U> other; };
template<class U> inline trace_allocator (const trace_allocator<U>& a): vm(a.vm) {}
inline trace_allocator (QVM& vm0): vm(vm0) {}
inline pointer allocate (const size_type n) { return reinterpret_cast<pointer>(vm_alloc(vm, n*sizeof(T))); }
inline void deallocate (pointer p, size_t n) { vm_dealloc(vm, p, n*sizeof(T)); }
};

template <class T, class U> inline bool operator== (const trace_allocator<T>& a, const trace_allocator<U>& b) { return &a.vm == &b.vm; }
template <class T, class U> inline bool operator!= (const trace_allocator<T>& a, const trace_allocator<U>& b) { return &a.vm != &b.vm; }

#endif
