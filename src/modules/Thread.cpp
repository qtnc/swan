#include "../include/Swan.hpp"
#include "../include/SwanBinding.hpp"
#include<thread>
#include<windows.h>
using namespace std;

struct SwanThread {
thread thr;
~SwanThread(){ }
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

void threadSleep (Swan::Fiber& f) {
int ms = f.getNum(0);
Swan::ScopeUnlocker unlocker(f.getVM());
Sleep(ms);
}

void threadJoin (Swan::Fiber& f) {
SwanThread& t = f.getUserObject<SwanThread>(0);
Swan::ScopeUnlocker unlocker(f.getVM());
t.thr.join();
}

void swanLoadThread (Swan::Fiber& f) {
f.pushNewMap();

f.pushNewClass<SwanThread>("Thread");
f.registerStaticMethod("()", threadLaunch);
f.registerMethod("join", threadJoin);
f.registerDestructor<SwanThread>();
f.storeIndex(-2, "Thread");
f.pop();

f.pushNativeFunction(threadSleep);
f.storeIndex(-2, "sleep");
f.pop();
}

