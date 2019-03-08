#ifndef _____SWAN_TUPLE_HPP_____
#define _____SWAN_TUPLE_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"

struct QTuple: QSequence {
size_t length;
QV data[];
QTuple (QVM& vm, uint32_t len): QSequence(vm.tupleClass), length(len) {}
inline QV& at (int n) {
if (n<0) n+=length;
return data[n];
}
static QTuple* create (QVM& vm, size_t length, const QV* data);
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { v.insert(v.begin()+start, &data[0], &data[length]); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) final override;
virtual ~QTuple () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this) + sizeof(QV) * length; }
};

#endif
