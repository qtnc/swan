#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_LINKED_LIST_HPP_____
#define _____SWAN_LINKED_LIST_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"
#include<list>


struct QLinkedList: QSequence {
typedef std::list<QV> list_type;
typedef list_type::iterator iterator;
list_type data;
QLinkedList (QVM& vm): QSequence(vm.linkedListClass) {}
inline QV& at (int n) {
int size = data.size();
iterator origin = data.begin();
if (n<0) origin = data.end();
else if (n>=size/2) { origin=data.end(); n-=size; }
std::advance(origin, n);
return *origin;
}
virtual void insertIntoVector (QFiber& f, std::vector<QV>& list, int start) override { list.insert(list.begin()+start, data.begin(), data.end()); }
virtual void insertIntoSet (QFiber& f, QSet& s) override { s.set.insert(data.begin(), data.end()); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QLinkedList () = default;
virtual bool gcVisit () override;
};

struct QLinkedListIterator: QObject {
QLinkedList& list;
QLinkedList::iterator iterator;
QLinkedListIterator (QVM& vm, QLinkedList& m): QObject(vm.objectClass), list(m), iterator(m.data.begin()) {}
virtual bool gcVisit () override;
virtual ~QLinkedListIterator() = default;
};
#endif
#endif
