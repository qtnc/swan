#ifndef ____VLS_____
#define ____VLS_____
#include<utility>

template<class T, class U, class... A> inline T* newVLS (int nU, A&&... args) {
char* ptr = new char[sizeof(T) + nU*sizeof(U)];
U* uPtr = reinterpret_cast<U*>(ptr + sizeof(T));
new(ptr) T( std::forward<A>(args)... );
new(uPtr) U[nU];
return reinterpret_cast<T*>(ptr);
}

#endif
