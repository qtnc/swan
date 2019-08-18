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
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { 
auto it = start<0? v.end() + start + 1 : v.begin() + start;
v.insert(it, data.begin(), data.end()); 
}
virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override {
auto it = start<0? data.end() + start + 1: data.begin() +  start;
data.insert(it, v.begin(), v.end());
version++;
}
virtual void join (QFiber& f, const std::string& delim, std::string& out) final override;
virtual ~QList () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QListIterator: QObject {
QList& list;
QList::iterator iterator;
uint32_t version;
bool forward;
QListIterator (QVM& vm, QList& m): QObject(vm.listIteratorClass), list(m), iterator(m.data.begin()), version(m.version), forward(false) {}
inline void incrVersion () { version++; list.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, list.version); }
virtual bool gcVisit () override;
virtual ~QListIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};
#endif
