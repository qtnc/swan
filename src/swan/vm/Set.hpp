#ifndef _____SWAN_SET_HPP_____
#define _____SWAN_SET_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include "VM.hpp"
#include "HasherAndEqualler.hpp"
#include<unordered_set>

void checkVersion(uint32_t,uint32_t);

struct QSet: QSequence {
typedef std::unordered_set<QV, QVHasher, QVEqualler, trace_allocator<QV>> set_type;
typedef set_type::iterator iterator;
set_type set;
uint32_t version;
QSet (QVM& vm);
inline void incrVersion () { version++; }
virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { set.insert(v.begin(), v.end()); incrVersion(); }
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { 
auto it = start<0? v.end() +start +1 : v.begin() + start;
v.insert(it, set.begin(), set.end());
}
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QSet () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QSetIterator: QObject {
QSet& set;
QSet::iterator iterator;
uint32_t version;
QSetIterator (QVM& vm, QSet& m): QObject(vm.setIteratorClass), set(m), iterator(m.set.begin()), version(m.version)  {}
virtual bool gcVisit () override;
virtual ~QSetIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
inline void incrVersion () { version++; set.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, set.version); }
};

#endif
