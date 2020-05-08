#include "TypeInfo.hpp"
#include "TypeAnalyzer.hpp"
#include "Expression.hpp"
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

string ClassTypeInfo::toBinString (QVM& vm) {
if (type==vm.numClass) return "N";
else if (type==vm.boolClass) return "B";
else if (type==vm.stringClass) return "S";
else if (type==vm.objectClass) return "O";
else if (type==vm.listClass) return "L";
else if (type==vm.tupleClass) return "T";
else if (type==vm.setClass) return "E";
else if (type==vm.mapClass) return "M";
else if (type==vm.undefinedClass) return "U";
else if (type==vm.functionClass) return "F";
else if (type==vm.iteratorClass) return "I";
else if (type==vm.iterableClass) return "A";
else return format("Q%s;", type->name);
}

shared_ptr<TypeInfo> ClassTypeInfo::merge (shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) {
if (!t0 || t0->isEmpty()) return shared_from_this();
t0 = t0->resolve(ta);
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

std::shared_ptr<TypeInfo> ComposedTypeInfo::resolve (TypeAnalyzer& ta) {
type = type? type->resolve(ta) :nullptr;
for (int i=0; i<nSubtypes; i++) subtypes[i] = subtypes[i]? subtypes[i]->resolve(ta) :nullptr;
return shared_from_this();
}

shared_ptr<TypeInfo> ComposedTypeInfo::merge (shared_ptr<TypeInfo> t0, TypeAnalyzer&  ta) {
if (!t0 || t0->isEmpty()) return shared_from_this();
auto t = dynamic_pointer_cast<ComposedTypeInfo>(t0->resolve(ta));
if (!t) return t0->merge(type, ta);
else if (t->nSubtypes!=nSubtypes) return t->type->merge(type, ta);
auto newType = type->merge(t->type, ta);
auto newSubtypes = make_unique<shared_ptr<TypeInfo>[]>(nSubtypes);
for (int i=0; i<nSubtypes; i++) newSubtypes[i] = ta.mergeTypes(subtypes[i], t->subtypes[i]);
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

string NamedTypeInfo::toBinString (QVM& vm) {
return format("Q%s;", string(token.start, token.length));
}

bool NamedTypeInfo::equals (const std::shared_ptr<TypeInfo>& other) { 
if (auto ni = std::dynamic_pointer_cast<NamedTypeInfo>(other)) return ni->token.length==token.length && 0==strncmp(ni->token.start, token.start, token.length); 
else return false; 
}

string ClassDeclTypeInfo::toBinString (QVM& vm) {
return format("Q%s;", string(cls->name.start, cls->name.length));
}


string ComposedTypeInfo::toBinString (QVM& vm) {
ostringstream out;
out << 'C';
out << type->toBinString(vm);
out << '<';
for (int i=0; i<nSubtypes; i++) out << subtypes[i]->toBinString(vm);
out << '>';
return out.str();
}

bool ComposedTypeInfo::equals (const std::shared_ptr<TypeInfo>& other) { 
auto ci = dynamic_pointer_cast<ComposedTypeInfo>(other);
if (!ci) return false;
if (!type || !ci->type) return false;
if (nSubtypes!=ci->nSubtypes) return false;
if (!type->equals(ci->type)) return false;
for (int i=0; i<nSubtypes; i++) {
if (!subtypes[i] || !ci->subtypes[i]) return false;
if (!subtypes[i]->equals(ci->subtypes[i])) return false;
}
return true;
}

