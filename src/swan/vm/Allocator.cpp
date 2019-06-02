#include "VM.hpp"
#include<vector>
#include<unordered_map>
#include<map>
#include<memory>
#include<boost/pool/pool.hpp>
#include "../../include/cpprintf.hpp"
using namespace std;

#define POOL_SIZE_MAX 256
#define RESERVE_SIZE_MAX 1048576

typedef boost::pool<> pool;
unordered_map<size_t, unique_ptr<pool>> bpools;
unordered_multimap<size_t,void*> freeList;

static inline pool& getpool (size_t n) {
auto it = bpools.find(n);
if (it==bpools.end()) it = bpools.insert(make_pair(n, make_unique<pool>(n, std::max<int>(32, 4096/n)))).first;
return *it->second;
}

static inline void round_upwards (size_t& n) {
n = ((n+7)/8)*8;
}

void purgeMem () {
for (auto& p: bpools) p.second->release_memory();
for (auto& p: freeList) free(p.second);
freeList.clear();
}


void* QVM::allocate (size_t n) {
gcMemUsage += n;
round_upwards(n);
if (n<=POOL_SIZE_MAX) return getpool(n).malloc();
else {
auto it = freeList.find(n);
if (it!=freeList.end()) {
void* ptr = it->second;
freeList.erase(it);
return ptr;
}
return malloc(n);
}}

void QVM::deallocate (void* p, size_t n) {
gcMemUsage -= n;
round_upwards(n);
if (n<=POOL_SIZE_MAX) getpool(n).free(p);
else if (n<=RESERVE_SIZE_MAX) freeList.insert(make_pair(n, p));
else free(p);
}
