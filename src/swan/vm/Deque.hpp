#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_DEQUE_HPP_____
#define _____SWAN_DEQUE_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"
#include "Allocator.hpp"
#include<deque>

struct QDeque: QSequence {
typedef std::deque<QV, trace_allocator<QV>> deque_type;
typedef deque_type::iterator iterator;
deque_type data;
QDeque (QVM& vm);
inline QV& at (int n) {
if (n<0) n+=data.size();
return data[n];
}
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { 
auto it = start<0? v.end() + start + 1 : v.begin() + start;
v.insert(it, data.begin(), data.end()); 
}
virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override {
auto it = start<0? data.end() + start + 1: data.begin() +  start;
data.insert(it, v.begin(), v.end());
}
virtual void join (QFiber& f, const std::string& delim, std::string& out) final override;
virtual ~QDeque () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QDequeIterator: QObject {
QDeque& deque;
QDeque::iterator iterator;
bool forward;
QDequeIterator (QVM& vm, QDeque& m): QObject(vm.dequeIteratorClass), deque(m), iterator(m.data.begin()), forward(false) {}
virtual bool gcVisit () override;
virtual ~QDequeIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};
#endif

#endif
