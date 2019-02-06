#ifndef _____SWAN_LIST_HPP_____
#define _____SWAN_LIST_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"
#include<vector>

struct QList: QSequence {
std::vector<QV> data;
QList (QVM& vm): QSequence(vm.listClass) {}
inline QV& at (int n) {
if (n<0) n+=data.size();
return data[n];
}
virtual void insertIntoVector (QFiber& f, std::vector<QV>& list, int start) override { list.insert(list.begin()+start, data.begin(), data.end()); }
virtual void insertIntoSet (QFiber& f, QSet& s) override { s.set.insert(data.begin(), data.end()); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QList () = default;
virtual bool gcVisit () override;
};

#endif
