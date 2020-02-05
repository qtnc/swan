#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_DICTIONARY_HPP_____
#define _____SWAN_DICTIONARY_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include "Allocator.hpp"
#include <map>

void checkVersion(uint32_t,uint32_t);

struct QDictionary: QSequence {
typedef std::multimap<QV, QV, QVBinaryPredicate, trace_allocator<std::pair<const QV,QV>>> map_type; 
typedef map_type::iterator iterator;
map_type map;
QV sorter;
uint32_t version;
QDictionary (struct QVM& vm, QV& sorter);
iterator get (const QV& key);
iterator getr (const QV& key);
void set (const QV& key, const QV& value);
~QDictionary () = default;
bool gcVisit ();
inline void incrVersion () { version++; }
inline size_t getMemSize () { return sizeof(*this); }
inline int getLength () { return map.size(); }
};

struct QDictionaryIterator: QObject {
QDictionary& map;
QDictionary::iterator iterator;
uint32_t version;
bool forward;
QDictionaryIterator (QVM& vm, QDictionary& m);
bool gcVisit ();
~QDictionaryIterator() = default;
inline void incrVersion () { version++; map.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, map.version); }
inline size_t getMemSize () { return sizeof(*this); }
};

#endif
#endif
