#include "VM.hpp"

void* vm_alloc (QVM& vm, size_t n) {
return vm.allocate(n);
}

void vm_dealloc (QVM& vm, void* p, size_t n) {
vm.deallocate(p, n);
}

Swan::VM& Swan::VM::create () {
return *new QVM();
}

int QVM::getOption (QVM::Option opt) {
switch(opt){
case Option::VAR_DECL_MODE: return varDeclMode;
case Option::COMPILATION_DEBUG_INFO: return compileDbgInfo;
case Option::GC_TRESHHOLD_FACTOR: return gcTreshholdFactor;
case Option::GC_TRESHHOLD: return gcTreshhold;
default: return 0;
}}

void QVM::setOption (QVM::Option opt, int value) {
switch(opt){
case Option::VAR_DECL_MODE: varDeclMode = value; break;
case Option::COMPILATION_DEBUG_INFO: compileDbgInfo = value; break;
case Option::GC_TRESHHOLD_FACTOR: gcTreshholdFactor = std::max(110, value); break;
case Option::GC_TRESHHOLD: gcTreshhold = std::max<size_t>(65536, std::max<size_t>(gcMemUsage + 16, value)); break;
}}

