#include "../include/Swan.hpp"
#include "../include/SwanBinding.hpp"
#include<thread>
#include<chrono>
using namespace std;

struct SwanThread {
thread thr;
~SwanThread(){ if (thr.joinable()) thr.detach(); }
void join () { if (thr.joinable()) thr.join(); }
};

static void threadLaunch (Swan::Fiber& f) {
Swan::VM& vm = f.getVM();
Swan::Handle func = f.getHandle(1);
thread t([=,&vm]()mutable{
Swan::ScopeLocker locker(vm);
Swan::Fiber& fb = vm.createFiber();
fb.pushHandle(func);
fb.call(0);
fb.release();
});
f.setEmplaceUserObject<SwanThread>(0);
f.getUserObject<SwanThread>(0) .thr = move(t);
}

void threadSleep (double time) {
std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000*time)));
}

void swanLoadThread (Swan::Fiber& f) {
f.pushNewMap();

f.pushNewClass<SwanThread>("Thread");
f.registerStaticMethod("()", threadLaunch);
f.registerMethod("join", UNLOCKED_METHOD(SwanThread, join));
f.registerDestructor<SwanThread>();
f.storeIndex(-2, "Thread");
f.pop();

f.pushNativeFunction(UNLOCKED_FUNCTION(threadSleep));
f.storeIndex(-2, "sleep");
f.pop();
}

