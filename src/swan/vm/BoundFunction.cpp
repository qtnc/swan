#include "Core.hpp"
#include "Object.hpp"
#include "BoundFunction.hpp"
#include "VM.hpp"
#include "../../include/cpprintf.hpp"

BoundFunction::BoundFunction (QVM& vm, const QV& m, size_t c):
QObject(vm.boundFunctionClass), method(m), count(c) {}

BoundFunction* BoundFunction::create (QVM& vm, const QV& m, size_t c, const QV* a) {
auto bf = vm.constructVLS<BoundFunction, QV>(c, vm, m, c);
memcpy(bf->args, a, c*sizeof(QV));
return bf;
}

