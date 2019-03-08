#ifndef _____SWAN_LIST_HPP_____
#define _____SWAN_LIST_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"
#include "Allocator.hpp"
#include<vector>

struct QList: QSequence {
std::vector<QV, trace_allocator<QV>> data;
QList (QVM& vm);
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
}
virtual void join (QFiber& f, const std::string& delim, std::string& out) final override;
virtual ~QList () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
