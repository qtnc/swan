#include "VM.hpp"
#include<vector>
#include<unordered_map>
//#include<boost/align/aligned_alloc.hpp>
#include "../../include/cpprintf.hpp"
using namespace std;

#define ALIGNMENT 8
#define RESERVE_SIZE_MAX 4096

unordered_multimap<size_t,void*> reserve;

void purgeMem ();

static inline char* galloc1 (size_t size) {
return static_cast<char*>(::malloc(size)); //static_cast<char*>(boost::alignment::aligned_alloc(ALIGNMENT, size));
}

static inline char* galloc (size_t size) {
char* p = galloc1(size);
if (!p) {
purgeMem();
p = galloc1(size);
}
return p;
}

static inline void gfree (void* ptr) {
//boost::alignment::aligned_free(ptr);
::free(ptr);
}

static inline void round_upwards (size_t& n) {
n = ((n +ALIGNMENT -1)/ALIGNMENT)*ALIGNMENT;
}

void purgeMem () {
for (auto& p: reserve) gfree(p.second);
reserve.clear();
}

void* QVM::allocate (size_t n) {
gcMemUsage += n;
round_upwards(n);
if (n<=RESERVE_SIZE_MAX) {
auto it = reserve.find(n);
if (it!=reserve.end()) {
void* ptr = it->second;
reserve.erase(it);
return ptr;
}}
return galloc(n);
}

void QVM::deallocate (void* p, size_t n) {
gcMemUsage -= n;
round_upwards(n);
if (n<=RESERVE_SIZE_MAX) reserve.insert(make_pair(n, p));
else  gfree(p);
}
