#include "Core.hpp"
#include "Object.hpp"
#include "StdFunction.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"

StdFunction::StdFunction (QVM& vm, const StdFunction::Func& func0):
QObject(vm.stdFunctionClass), func(func0) {}
