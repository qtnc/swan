#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_PRIORITY_QUEUE_HPP_____
#define _____SWAN_PRIORITY_QUEUE_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include "VM.hpp"
#include "Allocator.hpp"
#include <vector>
#include<algorithm>

void checkVersion(uint32_t,uint32_t);

struct QHeap: QSequence {
typedef std::vector<QV, trace_allocator<QV>> container_type;
typedef container_type::iterator iterator;
container_type  data;
QV sorter;
uint32_t version;
inline void incrVersion () { version++; }
QHeap (struct QVM& vm, QV& sorter):
QSequence(vm.heapClass), 
data(trace_allocator<QV>(vm)),
sorter(sorter), version(0)
{}
virtual bool gcVisit () override;
virtual ~QHeap () = default;

virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override {
data.insert(data.end(), v.begin(), v.end());
std::make_heap(data.begin(), data.end(), QVBinaryPredicate(type->vm, sorter));
incrVersion();
}
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { 
auto it = start<0? v.end() +start +1 : v.begin() + start;
v.insert(it, data.begin(), data.end());
}

virtual size_t getMemSize () override { return sizeof(*this); }

inline void push (const QV& x) {
data.push_back(x);
std::push_heap(data.begin(), data.end(), QVBinaryPredicate(type->vm, sorter));
incrVersion();
}

inline QV pop () {
if (!data.size()) return QV::UNDEFINED;
std::pop_heap(data.begin(), data.end(), QVBinaryPredicate(type->vm, sorter));
QV re = data.back();
data.pop_back();
incrVersion();
return re;
}

inline void erase (const container_type::iterator& it) {
data.erase(it);
std::make_heap(data.begin(), data.end(), QVBinaryPredicate(type->vm, sorter));
incrVersion();
}

};

struct QHeapIterator: QObject {
QHeap& heap;
QHeap::iterator iterator;
uint32_t version;
QHeapIterator (QVM& vm, QHeap& m): QObject(vm.heapIteratorClass), heap(m), iterator(m.data.begin()), version(m.version)  {}
virtual bool gcVisit () override;
virtual ~QHeapIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
inline void incrVersion () { version++; heap.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, heap.version); }
};
#endif
#endif
