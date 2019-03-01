#ifndef _____SWAN_MUTEX_HPP_____
#define _____SWAN_MUTEX_HPP_____
#include<atomic>
#include<mutex>
#include<thread>

#define LOCK_SCOPE(M) std::lock_guard<Mutex> ___mutexLock_##__LINE__(M);

#ifdef USE_STD_MUTEX
#include<mutex>
typedef std::recursive_mutex Mutex;
#else // USE_STD_MUTEX
#define SPIN_MAX 256

typedef struct SpinRecursiveMutex {
std::atomic_flag flag;
std::thread::id holdingThread;
uint16_t recurseCount;
inline SpinRecursiveMutex (): flag(ATOMIC_FLAG_INIT), holdingThread(), recurseCount(0) {}
inline void lock () { 
std::size_t spinCount = 0;
auto threadId = std::this_thread::get_id();
if (threadId == holdingThread) { ++recurseCount; return; }
while(flag.test_and_set(std::memory_order_acq_rel)) {
if (++spinCount%SPIN_MAX) std::this_thread::yield();
}
holdingThread = threadId;
recurseCount = 1;
}
inline bool try_lock () { 
auto threadId = std::this_thread::get_id();
if (threadId == holdingThread) { ++recurseCount; return true; }
bool locked = !flag.test_and_set(std::memory_order_acq_rel); 
if (locked) { holdingThread=threadId; recurseCount=1; }
return locked;
}
inline void unlock () { 
if (!--recurseCount) {
holdingThread = std::thread::id();
flag.clear(std::memory_order_release); 
}}
} Mutex;
#endif
#endif
