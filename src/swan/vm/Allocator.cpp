#include "VM.hpp"
#include<vector>
#include<unordered_map>
#include<memory>
using namespace std;

unordered_multimap<size_t,void*> freeList;



void* QVM::allocate (size_t n) {
gcMemUsage += n;
//return pool.ordered_malloc((n + POOL_ALLOC_SIZE -1) / POOL_ALLOC_SIZE);
//return malloc(n);
//return new char[n];
/*auto it = freeList.find(n);
if (it!=freeList.end()) {
void* ptr = it->second;
freeList.erase(it);
return ptr;
}*/
return malloc(n);
}

void QVM::deallocate (void* p, size_t n) {
gcMemUsage -= n;
//pool.ordered_free(p, (n+POOL_ALLOC_SIZE -1)/POOL_ALLOC_SIZE);
free(p);
//delete[] reinterpret_cast<char*>(p);
//freeList.insert(make_pair(n,p));
}
