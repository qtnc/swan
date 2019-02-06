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
virtual void insertIntoVector (QFiber& f, std::vector<QV>& list, int start) override { list.insert(list.begin()+start, data, data+length); }
virtual void insertIntoSet (QFiber& f, QSet& set) override { set.set.insert(data, data+length); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QTuple () = default;
virtual bool gcVisit () override;
};

#endif
