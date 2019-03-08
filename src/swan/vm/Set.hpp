#ifndef _____SWAN_SET_HPP_____
#define _____SWAN_SET_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include "VM.hpp"
#include "HasherAndEqualler.hpp"
#include<unordered_set>

struct QSet: QSequence {
typedef std::unordered_set<QV, QVHasher, QVEqualler, trace_allocator<QV>> set_type;
typedef set_type::iterator iterator;
set_type set;
QSet (QVM& vm);
virtual void insertFrom (QFiber& f, std::vector<QV, trace_allocator<QV>>& v, int start = -1) final override { set.insert(v.begin(), v.end()); }
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
QSetIterator (QVM& vm, QSet& m): QObject(vm.objectClass), set(m), iterator(m.set.begin()) {}
virtual bool gcVisit () override;
virtual ~QSetIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
