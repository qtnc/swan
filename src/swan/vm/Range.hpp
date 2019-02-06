#ifndef _____SWAN_RANGE_HPP_____
#define _____SWAN_RANGE_HPP_____
#include "Sequence.hpp"
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
};

#endif
