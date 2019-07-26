#ifndef NO_OPTIONAL_COLLECTIONS
#include "Deque.hpp"
#include "FiberVM.hpp"
#include "String.hpp"
using namespace std;

QDeque::QDeque (QVM& vm): 
QSequence(vm.dequeClass),
data(trace_allocator<QV>(vm))
{}

void QDeque::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV& x: data) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}}

#endif

