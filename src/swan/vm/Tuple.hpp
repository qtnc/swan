#ifndef _____SWAN_TUPLE_HPP_____
#define _____SWAN_TUPLE_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "VM.hpp"

struct QTuple: QSequence {
size_t length;
QV data[];
QTuple (QVM& vm, uint32_t len): QSequence(vm.tupleClass), length(len) {}
inline QV& at (int n) {
if (n<0) n+=length;
return data[n];
}
static QTuple* create (QVM& vm, size_t length, const QV* data);
bool join (QFiber& f, const std::string& delim, std::string& out);
~QTuple () = default;
bool gcVisit () ;
inline size_t getMemSize () { return sizeof(*this) + sizeof(QV) * length; }
inline QV* begin () { return data; }
inline QV* end () { return data+length; }
inline bool copyInto (QFiber& f, CopyVisitor& out) { std::for_each(begin(), end(), std::ref(out)); return true; }
inline int getLength () { return length; }
};

struct QTupleIterator: QObject {
QTuple& tuple;
const QV* iterator;
QTupleIterator (QVM& vm, QTuple& m): QObject(vm.tupleIteratorClass), tuple(m), iterator(m.begin()) {}
bool gcVisit () ;
~QTupleIterator() = default;
};

#endif
