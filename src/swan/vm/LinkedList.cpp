#ifndef NO_OPTIONAL_COLLECTIONS
#include "LinkedList.hpp"
#include "String.hpp"
#include "FiberVM.hpp"
using namespace std;

QLinkedList::QLinkedList (QVM& vm): 
QSequence(vm.linkedListClass),
data(trace_allocator<QV>(vm))
{}

void QLinkedList::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV& x: data) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}}
#endif

