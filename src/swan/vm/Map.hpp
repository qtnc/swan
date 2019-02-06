#ifndef _____SWAN_MAP_HPP_____
#define _____SWAN_MAP_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include <unordered_map>

struct QMap: QSequence {
typedef std::unordered_map<QV, QV, QVHasher, QVEqualler> map_type;
typedef map_type::iterator iterator;
map_type map;
QMap (QVM& vm): QSequence(vm.mapClass) {}
inline QV get (const QV& key) {
auto it = map.find(key);
if (it==map.end()) return QV();
else return it->second;
}
inline QV& set (const QV& key, const QV& value) { return map[key] = value; }
virtual ~QMap () = default;
virtual bool gcVisit () override;
};

struct QMapIterator: QObject {
QMap& map;
QMap::iterator iterator;
QMapIterator (QVM& vm, QMap& m): QObject(vm.objectClass), map(m), iterator(m.map.begin()) {}
virtual bool gcVisit () override;
virtual ~QMapIterator() = default;
};

#endif
