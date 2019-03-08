#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_LINKED_LIST_HPP_____
#define _____SWAN_LINKED_LIST_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"
#include "Allocator.hpp"
#include<list>


struct QLinkedList: QSequence {
typedef std::list<QV, trace_allocator<QV>> list_type;
typedef list_type::iterator iterator;
list_type data;
QLinkedList (QVM& vm);
inline QV& at (int n) {
int size = data.size();
iterator origin = data.begin();
if (n<0) origin = data.end();
else if (n>=size/2) { origin=data.end(); n-=size; }
std::advance(origin, n);
return *origin;
}
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { data.insert(data.end(), v.begin(), v.end()); }
virtual ~QLinkedList () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QLinkedListIterator: QObject {
QLinkedList& list;
QLinkedList::iterator iterator;
QLinkedListIterator (QVM& vm, QLinkedList& m): QObject(vm.objectClass), list(m), iterator(m.data.begin()) {}
virtual bool gcVisit () override;
virtual ~QLinkedListIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};
#endif
#endif
