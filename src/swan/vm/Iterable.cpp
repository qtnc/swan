#include "Iterable.hpp"
#include "FiberVM.hpp"
#include "Value.hpp"
#include "String.hpp"
#include<vector>
using namespace std;

bool QSequence::copyInto (QFiber& f, CopyVisitor& out) {
iterateSequence(f, QV(this), [&](const QV& x){ out(x); });
return true;
}

bool QSequence::join (QFiber& f, const string& delim, string& re) {
bool notFirst = false;
iterateSequence(f, QV(this), [&](const QV& x){ 
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
 });
return true;
}


bool QV::copyInto (QFiber& f, CopyVisitor& cv) const {
auto obj = asObject<QObject>();
return obj->type->gcInfo->copyInto(obj, f, cv);
}

