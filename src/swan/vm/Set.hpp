#ifndef _____SWAN_SET_HPP_____
#define _____SWAN_SET_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "HasherAndEqualler.hpp"
#include<unordered_set>

struct QSet: QSequence {
typedef std::unordered_set<QV, QVHasher, QVEqualler> set_type;
typedef set_type::iterator iterator;
set_type set;
QSet (QVM& vm);
virtual void insertIntoVector (QFiber& f, std::vector<QV>& list, int start) override { list.insert(list.begin()+start, set.begin(), set.end()); }
virtual void insertIntoSet (QFiber& f, QSet& s) override { s.set.insert(set.begin(), set.end()); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QSet () = default;
virtual bool gcVisit () override;
};

struct QSetIterator: QObject {
QSet& set;
QSet::iterator iterator;
QSetIterator (QVM& vm, QSet& m): QObject(vm.objectClass), set(m), iterator(m.set.begin()) {}
virtual bool gcVisit () override;
virtual ~QSetIterator() = default;
};

#endif
