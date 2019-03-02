#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_PRIORITY_QUEUE_HPP_____
#define _____SWAN_PRIORITY_QUEUE_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include "VM.hpp"
#include <vector>
#include<algorithm>

struct QPriorityQueue: QSequence {
std::vector<QV> data;
QV sorter;
QPriorityQueue (struct QVM& vm, QV& sorter0):
QSequence(vm.priorityQueueClass), sorter(sorter0) {}
virtual bool gcVisit () override;
virtual ~QPriorityQueue () = default;

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

inline void erase (std::vector<QV>::iterator it) {
data.erase(it);
std::make_heap(data.begin(), data.end(), QVBinaryPredicate(type->vm, sorter));
}

};
#endif
#endif
