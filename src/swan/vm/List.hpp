#ifndef _____SWAN_LIST_HPP_____
#define _____SWAN_LIST_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"
#include "Allocator.hpp"
#include<vector>

void checkVersion (uint32_t v1, uint32_t v2);

struct QList: QSequence {
typedef std::vector<QV, trace_allocator<QV>> vector_type;
typedef vector_type::iterator iterator;
vector_type data;
uint32_t version;
QList (QVM& vm);
inline void incrVersion () { version++; }
inline QV& at (int n) {
if (n<0) n+=data.size();
return data[n];
}
inline bool copyInto (QFiber& f, CopyVisitor& out) { std::for_each(data.begin(), data.end(), std::ref(out)); return true; }
inline int getLength () { return data.size(); }
bool join (QFiber& f, const std::string& delim, std::string& out);
~QList () = default;
bool gcVisit ();
inline size_t getMemSize () { return sizeof(*this); }
};

struct QListIterator: QObject {
QList& list;
QList::iterator iterator;
uint32_t version;
bool forward;
QListIterator (QVM& vm, QList& m): QObject(vm.listIteratorClass), list(m), iterator(m.data.begin()), version(m.version), forward(false) {}
inline void incrVersion () { version++; list.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, list.version); }
bool gcVisit ();
~QListIterator() = default;
inline size_t getMemSize () { return sizeof(*this); }
};
#endif
