#ifndef NO_OPTIONAL_COLLECTIONS
#ifndef _____SWAN_DICTIONARY_HPP_____
#define _____SWAN_DICTIONARY_HPP_____
#include "Sequence.hpp"
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include <map>

struct QDictionary: QSequence {
typedef std::multimap<QV, QV, QVBinaryPredicate> map_type; 
typedef map_type::iterator iterator;
map_type map;
QV sorter;
QDictionary (struct QVM& vm, QV& sorter0);
iterator get (const QV& key);
iterator getr (const QV& key);
void set (const QV& key, const QV& value);
virtual ~QDictionary () = default;
virtual bool gcVisit () override;
};

struct QDictionaryIterator: QObject {
QDictionary& map;
QDictionary::iterator iterator;
QDictionaryIterator (QVM& vm, QDictionary& m);
virtual bool gcVisit () override;
virtual ~QDictionaryIterator() = default;
};

#endif
#endif
