#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_LINKED_LIST_HPP_____
#define _____SWAN_LINKED_LIST_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"
#include "Allocator.hpp"
#include<list>

void checkVersion (uint32_t v1, uint32_t v2);


struct QLinkedList: QSequence {
typedef std::list<QV, trace_allocator<QV>> list_type;
typedef list_type::iterator iterator;
list_type data;
uint32_t version;
QLinkedList (QVM& vm);
inline void incrVersion () { version++; }
inline QV& at (int n) {
int size = data.size();
iterator origin = data.begin();
if (n<0) origin = data.end();
else if (n>=size/2) { origin=data.end(); n-=size; }
std::advance(origin, n);
return *origin;
}
bool join (QFiber& f, const std::string& delim, std::string& out);
inline bool copyInto (QFiber& f, CopyVisitor& out) { std::for_each(data.begin(), data.end(), std::ref(out)); return true; }
inline int getLength () { return data.size(); }
~QLinkedList () = default;
bool gcVisit ();
inline size_t getMemSize () { return sizeof(*this); }
};

struct QLinkedListIterator: QObject {
QLinkedList& list;
QLinkedList::iterator iterator;
uint32_t version;
bool forward;
QLinkedListIterator (QVM& vm, QLinkedList& m): QObject(vm.linkedListIteratorClass), list(m), iterator(m.data.begin()), version(m.version), forward(false)  {}
inline void incrVersion () { version++; list.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, list.version); }
bool gcVisit () ;
~QLinkedListIterator() = default;
inline size_t getMemSize () { return sizeof(*this); }
};
#endif
#endif
