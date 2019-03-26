#ifndef _____SWAN_RANGE_HPP_____
#define _____SWAN_RANGE_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "VM.hpp"

struct QRange: QSequence, Swan::Range  {
QRange (QVM& vm, double s, double e, double p, bool i): QSequence(vm.rangeClass), Swan::Range(s, e, p, i) {}
QRange (QVM& vm, const Swan::Range& r): QSequence(vm.rangeClass), Swan::Range(r) {}
QV iterate (const QV& x) {
if (x.isNull()) return start;
double val = x.asNum() + step;
if (inclusive) return (end-start) * (end -val) >= 0 ? val : QV();
else return (end-start) * (end -val) > 0 ? val : QV();
}
virtual ~QRange () = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QRangeIterator: QObject {
QRange& range;
double value;
QRangeIterator (QVM& vm, QRange& m): QObject(vm.objectClass), range(m), value(m.start)  {}
virtual bool gcVisit () override;
virtual ~QRangeIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};
#endif
