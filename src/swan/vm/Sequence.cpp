#include "Sequence.hpp"
#include "FiberVM.hpp"
#include "Value.hpp"
#include "String.hpp"
#include "Set.hpp"
#include<vector>
using namespace std;

void QSequence::insertIntoVector (QFiber& f, std::vector<QV>& list, int start) {
vector<QV> toInsert;
iterateSequence(f, QV(this), [&](const QV& x){ toInsert.push_back(x); });
if (!toInsert.empty()) list.insert(list.begin()+start, toInsert.begin(), toInsert.end());
}

void QSequence::insertIntoSet (QFiber& f, QSet& set) {
vector<QV> toInsert;
iterateSequence(f, QV(this), [&](const QV& x){ toInsert.push_back(x); });
if (!toInsert.empty()) set.set.insert(toInsert.begin(), toInsert.end());
}

void QSequence::join (QFiber& f, const string& delim, string& re) {
bool notFirst = false;
iterateSequence(f, QV(this), [&](const QV& x){ 
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
 });
}
