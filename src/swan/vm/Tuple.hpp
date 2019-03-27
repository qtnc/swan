#ifndef _____SWAN_TUPLE_HPP_____
#define _____SWAN_TUPLE_HPP_____
#include "Iterable.hpp"
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
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { 
auto it = start<0? v.end() +start +1 : v.begin() + start;
v.insert(it, &data[0], &data[length]); 
}
virtual void join (QFiber& f, const std::string& delim, std::string& out) final override;
virtual ~QTuple () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this) + sizeof(QV) * length; }
inline QV* begin () { return data; }
inline QV* end () { return data+length; }
};

struct QTupleIterator: QObject {
QTuple& tuple;
const QV* iterator;
QTupleIterator (QVM& vm, QTuple& m): QObject(vm.tupleIteratorClass), tuple(m), iterator(m.begin()) {}
virtual bool gcVisit () override;
virtual ~QTupleIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
