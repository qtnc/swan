#ifndef _____SWAN_MUTEX_HPP_____
#define _____SWAN_MUTEX_HPP_____
#ifndef NO_THREAD_SUPPORT
#define LOCK_SCOPE(M) std::lock_guard<decltype(M)> ___mutexLock_##__LINE__(M);
#include<mutex>
typedef std::recursive_mutex GIL;
#else
#define LOCK_SCOPE(M) /* do nothing */
struct GIL {
inline void lock () {}
inline void unlock () {}
inline bool try_lock () { return true; }
};
#endif
#endif
