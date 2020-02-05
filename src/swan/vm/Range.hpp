#ifndef _____SWAN_RANGE_HPP_____
#define _____SWAN_RANGE_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "VM.hpp"

struct QRange: QSequence, Swan::Range  {
QRange (QVM& vm, double s, double e, double p, bool i): QSequence(vm.rangeClass), Swan::Range(s, e, p, i) {}
QRange (QVM& vm, const Swan::Range& r): QSequence(vm.rangeClass), Swan::Range(r) {}
~QRange () = default;
inline size_t getMemSize () { return sizeof(*this); }
inline int getLength () {  return ((end-start)/step) + inclusive?1:0; }
};

struct QRangeIterator: QObject {
QRange& range;
double value;
QRangeIterator (QVM& vm, QRange& m): QObject(vm.rangeIteratorClass), range(m), value(m.start)  {}
bool gcVisit ();
~QRangeIterator() = default;
inline size_t getMemSize () { return sizeof(*this); }
};
#endif
