#include "Sequence.hpp"
#include "FiberVM.hpp"
#include "Value.hpp"
#include "String.hpp"
#include "Set.hpp"
#include<vector>
using namespace std;

void QSequence::insertIntoVector (QFiber& f, std::vector<QV>& list, int start) {
iterateSequence(f, QV(this), [&](const QV& x){ list.insert(list.begin()+start++, x); });
}

void QSequence::insertIntoSet (QFiber& f, QSet& set) {
iterateSequence(f, QV(this), [&](const QV& x){ set.set.insert(x); });
}

void QSequence::join (QFiber& f, const string& delim, string& re) {
bool notFirst = false;
iterateSequence(f, QV(this), [&](const QV& x){ 
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
 });
}

