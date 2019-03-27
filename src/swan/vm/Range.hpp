#ifndef _____SWAN_RANGE_HPP_____
#define _____SWAN_RANGE_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "VM.hpp"

struct QRange: QSequence, Swan::Range  {
QRange (QVM& vm, double s, double e, double p, bool i): QSequence(vm.rangeClass), Swan::Range(s, e, p, i) {}
QRange (QVM& vm, const Swan::Range& r): QSequence(vm.rangeClass), Swan::Range(r) {}
virtual ~QRange () = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QRangeIterator: QObject {
QRange& range;
double value;
QRangeIterator (QVM& vm, QRange& m): QObject(vm.rangeIteratorClass), range(m), value(m.start)  {}
virtual bool gcVisit () override;
virtual ~QRangeIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};
#endif
