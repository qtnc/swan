#include "List.hpp"
#include "FiberVM.hpp"
#include "String.hpp"
using namespace std;

void QList::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV& x: data) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}}

