#ifndef NO_OPTIONAL_COLLECTIONS
#include "Dictionary.hpp"
#include "HasherAndEqualler.hpp"
#include "VM.hpp"
using namespace std;

QDictionaryIterator::QDictionaryIterator (QVM& vm, QDictionary& m): 
QObject(vm.objectClass), map(m), iterator(m.map.begin()) 
{}

QDictionary::QDictionary (QVM& vm, QV& sorter0): 
QSequence(vm.dictionaryClass), map(sorter0), sorter(sorter0) 
{}

QDictionary::iterator QDictionary::get (const QV& key) {
auto range = map.equal_range(key);
if (range.first==range.second) return map.end();
QVEqualler eq;
auto it = find_if(range.first, range.second, [&](const auto& i){ return eq(i.first, key); });
if (it!=range.second) return it;
else return map.end();
}

QDictionary::iterator QDictionary::getr (const QV& key) {
auto it = get(key);
if (it==map.end()) it = map.insert(make_pair(key, QV()));
return it;
}

void QDictionary::set (const QV& key, const QV& value) { 
getr(key)->second = value;
}

#endif
