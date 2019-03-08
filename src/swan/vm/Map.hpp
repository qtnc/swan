#ifndef _____SWAN_MAP_HPP_____
#define _____SWAN_MAP_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include "Allocator.hpp"
#include <unordered_map>

struct QMap: QSequence {
typedef std::unordered_map<QV, QV, QVHasher, QVEqualler, trace_allocator<std::pair<const QV, QV>>> map_type;
typedef map_type::iterator iterator;
map_type map;
QMap (QVM& vm);
inline QV get (const QV& key) {
auto it = map.find(key);
if (it==map.end()) return QV();
else return it->second;
}
inline QV& set (const QV& key, const QV& value) { return map[key] = value; }
virtual ~QMap () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QMapIterator: QObject {
QMap& map;
QMap::iterator iterator;
QMapIterator (QVM& vm, QMap& m);
virtual bool gcVisit () override;
virtual ~QMapIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
