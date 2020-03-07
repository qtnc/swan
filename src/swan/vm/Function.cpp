#include "Core.hpp"
//#include "Upvalue.hpp"
#include "Object.hpp"
#include "Function.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"

QFunction::QFunction (QVM& vm): 
QObject(vm.functionClass), 
nArgs(0), vararg(false), 
iField(0), fieldGetter(false), fieldSetter(false),
upvalues(nullptr), bytecode(nullptr), bytecodeEnd(nullptr)
{}

QFunction* QFunction::create (QVM& vm, int nArgs, int nConsts, int nUpvalues, int bcSize, int nDebugItems) {
QFunction* function = vm.constructVLS<QFunction, char>(nConsts*sizeof(QV) + nUpvalues*sizeof(Upvariable) + nDebugItems*sizeof(DebugItem) + bcSize, vm);
function->constantsEnd = function->constants + nConsts;
function->upvaluesEnd = function->upvalues + nUpvalues;
function->bytecodeEnd = function->bytecode + bcSize;
function->debugItemsEnd = function->debugItems + nDebugItems;
function->nArgs = nArgs;
return function;
}

