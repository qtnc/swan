#include "VM.hpp"

QFiber& QVM::getActiveFiber () {
if (!QFiber::curFiber) {
LOCK_SCOPE(globalMutex)
auto f = new QFiber(*this);
fiberThreads.push_back(&QFiber::curFiber);
QFiber::curFiber=f;
}
return *QFiber::curFiber;
}

Swan::VM& export Swan::VM::getVM  () {
static QVM* vm = nullptr;
if (!vm) {
#ifdef NO_MUTEX
vm = new QVM();
#else
static std::once_flag onceFlag;
auto func = [&](){ vm = new QVM(); };
std::call_once(onceFlag, func);
#endif
}
return *vm;
}


int QVM::getOption (QVM::Option opt) {
switch(opt){
case Option::VAR_DECL_MODE: return varDeclMode;
default: return 0;
}}

void QVM::setOption (QVM::Option opt, int value) {
switch(opt){
case Option::VAR_DECL_MODE:
varDeclMode = value;
break;
}}

