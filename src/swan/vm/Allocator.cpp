#include "VM.hpp"
#include<vector>
#include<unordered_map>
#include<boost/pool/pool.hpp>
#include<boost/align/aligned_alloc.hpp>
#include "../../include/cpprintf.hpp"
using namespace std;

#define ALIGNMENT 8
#define POOL_SIZE_MAX 0
#define RESERVE_SIZE_MAX 1048576

struct aligned_user_allocator {
typedef size_t size_type;
typedef intptr_t difference_type;
static inline char* malloc (size_t size) {
return static_cast<char*>(boost::alignment::aligned_alloc(ALIGNMENT, size));
}
static inline void free (void* ptr) {
boost::alignment::aligned_free(ptr);
}
};

typedef boost::pool<aligned_user_allocator> pool;
unordered_map<size_t, unique_ptr<pool>> bpools;
unordered_multimap<size_t,void*> freeList;

static inline pool& getpool (size_t n) {
auto it = bpools.find(n);
if (it==bpools.end()) it = bpools.insert(make_pair(n, make_unique<pool>(n, std::max<int>(32, 4096/n)))).first;
return *it->second;
}

static inline void round_upwards (size_t& n) {
n = ((n +ALIGNMENT -1)/ALIGNMENT)*ALIGNMENT;
}

void purgeMem () {
//for (auto& p: bpools) p.second->release_memory(); //Fixme: disabled because release_memory() randomly crashes
for (auto& p: freeList) boost::alignment::aligned_free(p.second);
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
return boost::alignment::aligned_alloc(ALIGNMENT, n);
}}

void QVM::deallocate (void* p, size_t n) {
gcMemUsage -= n;
round_upwards(n);
memset(p, 0, n);
if (n<=POOL_SIZE_MAX) getpool(n).free(p);
else if (n<=RESERVE_SIZE_MAX) freeList.insert(make_pair(n, p));
else boost::alignment::aligned_free(p);
}
