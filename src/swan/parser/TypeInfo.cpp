#include "TypeInfo.hpp"
#include "Compiler.hpp"
#include "../vm/VM.hpp"
#include<sstream>
using namespace std;

shared_ptr<TypeInfo> TypeInfo::ANY = make_shared<AnyTypeInfo>();

shared_ptr<TypeInfo> TypeInfo::MANY = make_shared<ManyTypeInfo>();

bool ClassTypeInfo::isNum (QVM& vm) { 
return type==vm.numClass; 
}

bool ClassTypeInfo::isBool (QVM& vm) { 
return type==vm.boolClass; 
}

string ClassTypeInfo::toString () { 
return type->name.c_str(); 
}

shared_ptr<TypeInfo> ClassTypeInfo::merge (shared_ptr<TypeInfo> t0, QCompiler& compiler) {
if (!t0 || t0->isEmpty()) return shared_from_this();
t0 = t0->resolve(compiler);
auto t = dynamic_pointer_cast<ClassTypeInfo>(t0);
if (!t) return TypeInfo::MANY;
if (t->type==type) return shared_from_this();
QClass* cls = findCommonParent(type, t->type);
if (cls) return make_shared<ClassTypeInfo>(cls);
else return TypeInfo::MANY;
}

QClass* ClassTypeInfo::findCommonParent (QClass* t1, QClass* t2) {
if (t1==t2) return t1;
for (auto p1=t1; p1; p1=p1->parent) {
for (auto p2=t2; p2; p2=p2->parent) {
if (p1==p2) return p1;
}}
return nullptr;
}

ComposedTypeInfo::ComposedTypeInfo (shared_ptr<TypeInfo> tp, int nst, unique_ptr<shared_ptr<TypeInfo>[]>&& st): 
type(tp), nSubtypes(nst), 
subtypes(std::move(st)) 
{}

shared_ptr<TypeInfo> ComposedTypeInfo::merge (shared_ptr<TypeInfo> t0, QCompiler& compiler) {
if (!t0 || t0->isEmpty()) return shared_from_this();
auto t = dynamic_pointer_cast<ComposedTypeInfo>(t0->resolve(compiler));
if (!t) return t0->merge(type, compiler);
else if (t->nSubtypes!=nSubtypes) return t->type->merge(type, compiler);
auto newType = type->merge(t->type, compiler);
auto newSubtypes = make_unique<shared_ptr<TypeInfo>[]>(nSubtypes);
for (int i=0; i<nSubtypes; i++) newSubtypes[i] = subtypes[i]->merge(t->subtypes[i], compiler);
return make_shared<ComposedTypeInfo>(newType, nSubtypes, std::move(newSubtypes));
}

string ComposedTypeInfo::toString () {
ostringstream out;
out << type->toString() << '<';
for (int i=0; i<nSubtypes; i++) {
if (i>0) out << ',';
out << subtypes[i]->toString();
}
out << '>';
return out.str();
}

