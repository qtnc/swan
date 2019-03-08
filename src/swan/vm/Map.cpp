#include "Map.hpp"
#include "VM.hpp"
using namespace std;

QMapIterator::QMapIterator (QVM& vm, QMap& m): 
QObject(vm.objectClass), map(m), iterator(m.map.begin()) 
{}

QMap::QMap (QVM& vm): 
QSequence(vm.mapClass), 
map(4, QVHasher(vm), QVEqualler(vm), trace_allocator<pair<const QV, QV>>(vm)) 
{}
