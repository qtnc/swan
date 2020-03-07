#include "Core.hpp"
#include "Upvalue.hpp"
#include "Object.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"

Upvalue::Upvalue (QFiber& f, int slot): 
QObject(f.vm.objectClass), fiber(&f), closedValue(QV_OPEN_UPVALUE_MARK), value(&f.stack.at(stackpos(f, slot)))
{}

Upvalue::Upvalue (QFiber& f, const QV& v): 
QObject(f.vm.objectClass), fiber(&f), closedValue(v), value(&closedValue)
{}

