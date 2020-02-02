#include "Set.hpp"
#include "FiberVM.hpp"
#include "String.hpp"
using namespace std;

QSet::QSet (QVM& vm): 
QSequence(vm.setClass), 
set(4, QVHasher(vm), QVEqualler(vm), trace_allocator<QV>(vm)), version(0)
{}

bool QSet::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (const QV& x: set) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}
return true;
}

