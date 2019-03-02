#include "VM.hpp"

Swan::VM& Swan::VM::create () {
return *new QVM();
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

