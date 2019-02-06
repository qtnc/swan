#ifndef NO_OPTIONAL_COLLECTIONS
#include "LinkedList.hpp"
#include "String.hpp"
#include "FiberVM.hpp"
using namespace std;

void QLinkedList::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV& x: data) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}}
#endif

