#include "Tuple.hpp"
#include "FiberVM.hpp"
#include "String.hpp"
using namespace std;

QTuple* QTuple::create (QVM& vm, size_t length, const QV* data) {
QTuple* tuple = vm.constructVLS<QTuple, QV>(length, vm, length);
memcpy(tuple->data, data, length*sizeof(QV));
return tuple;
}

bool QTuple::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV *x = data, *end=data+length; x<end; x++) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, *x, re);
}
return true;
}

