#include "Map.hpp"
#include "VM.hpp"
using namespace std;

QMapIterator::QMapIterator (QVM& vm, QMap& m): 
QObject(vm.mapIteratorClass), map(m), iterator(m.map.begin()), version(m.version)
{}

QMap::QMap (QVM& vm): 
QSequence(vm.mapClass), 
map(4, QVHasher(vm), QVEqualler(vm), trace_allocator<pair<const QV, QV>>(vm)), version(0)
{
map.max_load_factor(0.75f);
}
