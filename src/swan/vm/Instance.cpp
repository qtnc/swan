#include "Instance.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"


QInstance* QInstance::create (QClass* type, int nFields) { 
return type->vm.constructVLS<QInstance, QV>(nFields, type); 
}

size_t QInstance::getMemSize () { 
return sizeof(*this) + sizeof(QV) * type->nFields; 
}
