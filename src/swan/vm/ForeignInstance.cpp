#include "ForeignInstance.hpp"
#include "ForeignClass.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"


QForeignInstance* QForeignInstance::create (QClass* type, int nBytes) { 
return type->vm.constructVLS<QForeignInstance, char>(nBytes, type); 
}

size_t QForeignInstance::getMemSize () { 
return sizeof(*this) + type->nFields; 
}

QForeignInstance::~QForeignInstance () {
QForeignClass* cls = static_cast<QForeignClass*>(type);
if (cls->destructor) cls->destructor(userData);
}
