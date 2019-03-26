#include "Iterable.hpp"
#include "FiberVM.hpp"
#include "Value.hpp"
#include "String.hpp"
#include "Set.hpp"
#include<vector>
using namespace std;

void QSequence::copyInto  (QFiber& f, std::vector<QV, trace_allocator<QV>>& list, int start) {
if (start<0) start += list.size() +1;
iterateSequence(f, QV(this), [&](const QV& x){ list.insert(list.begin()+start++, x); });
}

void QSequence::copyInto (QFiber& f, QSequence& seq) {
vector<QV, trace_allocator<QV>> v(f.vm);
copyInto(f, v);
seq.insertFrom(f, v);
}

void QSequence::insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start) {
throw std::runtime_error("Unsupported operation");
}

void QSequence::join (QFiber& f, const string& delim, string& re) {
bool notFirst = false;
iterateSequence(f, QV(this), [&](const QV& x){ 
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, x, re);
 });
}

