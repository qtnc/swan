#ifndef _____SWAN_MUTEX_HPP_____
#define _____SWAN_MUTEX_HPP_____

#ifndef NO_MUTEX
#include<mutex>
typedef std::recursive_mutex Mutex;
#define LOCK_SCOPE(M) std::lock_guard<Mutex> ___mutexLock_##__LINE__(M);
#else
struct Mutex { inline void lock () {} inline void unlock () {} };
#define LOCK_SCOPE(M) /* do nothing */
#endif

#endif
