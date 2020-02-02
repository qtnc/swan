#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_DEQUE_HPP_____
#define _____SWAN_DEQUE_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"
#include "Allocator.hpp"
#include<deque>

void checkVersion (uint32_t, uint32_t);

struct QDeque: QSequence {
typedef std::deque<QV, trace_allocator<QV>> deque_type;
typedef deque_type::iterator iterator;
deque_type data;
uint32_t version;
QDeque (QVM& vm);
inline void incrVersion () { version++; }
inline QV& at (int n) {
if (n<0) n+=data.size();
return data[n];
}
bool join (QFiber& f, const std::string& delim, std::string& out);
inline bool copyInto (QFiber& f, CopyVisitor& out) { std::for_each(data.begin(), data.end(), std::ref(out)); return true; }
~QDeque () = default;
bool gcVisit ();
inline size_t getMemSize () { return sizeof(*this); }
};

struct QDequeIterator: QObject {
QDeque& deque;
QDeque::iterator iterator;
uint32_t version;
bool forward;
QDequeIterator (QVM& vm, QDeque& m): QObject(vm.dequeIteratorClass), deque(m), iterator(m.data.begin()), version(m.version), forward(false) {}
inline void incrVersion () { version++; deque.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, deque.version); }
bool gcVisit ();
~QDequeIterator() = default;
inline size_t getMemSize () { return sizeof(*this); }
};
#endif

#endif
