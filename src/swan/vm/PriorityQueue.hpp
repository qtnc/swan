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

struct QPriorityQueue: QSequence {
typedef std::vector<QV, trace_allocator<QV>> container_type;
typedef container_type::iterator iterator;
container_type  data;
QV sorter;
QPriorityQueue (struct QVM& vm, QV& sorter0):
QSequence(vm.priorityQueueClass), 
data(trace_allocator<QV>(vm)),
sorter(sorter0)
{}
virtual bool gcVisit () override;
virtual ~QPriorityQueue () = default;

virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override {
data.insert(data.end(), v.begin(), v.end());
std::make_heap(data.begin(), data.end(), QVBinaryPredicate(type->vm, sorter));
}
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { 
auto it = start<0? v.end() +start +1 : v.begin() + start;
v.insert(it, data.begin(), data.end());
}

virtual size_t getMemSize () override { return sizeof(*this); }

inline void push (const QV& x) {
data.push_back(x);
std::push_heap(data.begin(), data.end(), QVBinaryPredicate(type->vm, sorter));
}

inline QV pop () {
if (!data.size()) return QV();
std::pop_heap(data.begin(), data.end(), QVBinaryPredicate(type->vm, sorter));
QV re = data.back();
data.pop_back();
return re;
}

inline void erase (const container_type::iterator& it) {
data.erase(it);
std::make_heap(data.begin(), data.end(), QVBinaryPredicate(type->vm, sorter));
}

};

struct QPriorityQueueIterator: QObject {
QPriorityQueue& pq;
QPriorityQueue::iterator iterator;
QPriorityQueueIterator (QVM& vm, QPriorityQueue& m): QObject(vm.priorityQueueIteratorClass), pq(m), iterator(m.data.begin()) {}
virtual bool gcVisit () override;
virtual ~QPriorityQueueIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};
#endif
#endif
