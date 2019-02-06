#include "Set.hpp"
#include "FiberVM.hpp"
#include "String.hpp"
using namespace std;

void QSet::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (const QV& x: set) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
}}

