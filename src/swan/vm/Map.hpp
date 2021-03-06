#ifndef _____SWAN_MAP_HPP_____
#define _____SWAN_MAP_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include "Allocator.hpp"
#include <unordered_map>

void checkVersion(uint32_t,uint32_t);

struct QMap: QSequence {
typedef std::unordered_map<QV, QV, QVHasher, QVEqualler, trace_allocator<std::pair<const QV, QV>>> map_type;
typedef map_type::iterator iterator;
map_type map;
uint32_t version;
QMap (QVM& vm);
inline void incrVersion () { version++; }
inline QV get (const QV& key) {
auto it = map.find(key);
if (it==map.end()) return QV::UNDEFINED;
else return it->second;
}
inline QV& set (const QV& key, const QV& value) { 
incrVersion();
return map[key] = value; 
}
virtual ~QMap () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this); }
};

struct QMapIterator: QObject {
QMap& map;
QMap::iterator iterator;
uint32_t version;
QMapIterator (QVM& vm, QMap& m);
virtual bool gcVisit () override;
virtual ~QMapIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
inline void incrVersion () { version++; map.incrVersion(); }
inline void checkVersion () { ::checkVersion(version, map.version); }
};

#endif
