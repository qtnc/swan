#include "Core.hpp"
#include "Object.hpp"
#include "Closure.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"

QClosure::QClosure (QVM& vm, QFunction& f):
QObject(vm.closureClass), func(f) {}

size_t QClosure::getMemSize () { 
return sizeof(*this) + sizeof(QV) * (func.upvaluesEnd - func.upvalues); 
}
