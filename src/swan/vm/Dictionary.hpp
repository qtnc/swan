#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_DICTIONARY_HPP_____
#define _____SWAN_DICTIONARY_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include <map>

struct QDictionary: QSequence {
typedef std::map<QV, QV, QVBinaryPredicate> map_type; 
typedef map_type::iterator iterator;
map_type map;
QV sorter;
QDictionary (QVM& vm, QV& sorter0): QSequence(vm.dictionaryClass), map(sorter0), sorter(sorter0) {}
inline QV get (const QV& key) {
auto it = map.find(key);
if (it==map.end()) return QV();
else return it->second;
}
inline QV& set (const QV& key, const QV& value) { return map[key] = value; }
virtual ~QDictionary () = default;
virtual bool gcVisit () override;
};

struct QDictionaryIterator: QObject {
QDictionary& map;
QDictionary::iterator iterator;
QDictionaryIterator (QVM& vm, QDictionary& m): QObject(vm.objectClass), map(m), iterator(m.map.begin()) {}
virtual bool gcVisit () override;
virtual ~QDictionaryIterator() = default;
};

#endif
#endif
