#include "List.hpp"
#include "FiberVM.hpp"
#include "String.hpp"
using namespace std;

QList::QList (QVM& vm): 
QSequence(vm.listClass),
data(trace_allocator<QV>(vm)),
version(0)
{}

bool QList::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV& x: data) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}
return true;
}

