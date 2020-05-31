#include "TypeInfo.hpp"
#include "TypeAnalyzer.hpp"
#include "Expression.hpp"
#include "../vm/VM.hpp"
#include<sstream>
using namespace std;

shared_ptr<TypeInfo> TypeInfo::ANY = make_shared<AnyTypeInfo>();
shared_ptr<TypeInfo> TypeInfo::MANY = make_shared<ManyTypeInfo>();

string SubindexTypeInfo::toString () {
return format("%c%d", index>=0x100? '@' : '%', index&0xFF);
}

string SubindexTypeInfo::toBinString (QVM& vm) {
return format("%c%d", index>=0x100? '@' : '%', index&0xFF);
}

ClassTypeInfo::ClassTypeInfo (QClass* c1, bool ex, bool op):
type(c1), exact(ex), optional(op) {
exact = exact || type->nonInheritable;
optional = optional && type!=type->vm.nullClass && type!=type->vm.undefinedClass;
}

bool ClassTypeInfo::isNum () { 
return type==type->vm.numClass; 
}

bool ClassTypeInfo::isBool () { 
return type==type->vm.boolClass; 
}

bool ClassTypeInfo::isString () { 
return type==type->vm.stringClass; 
}

bool ClassTypeInfo::isNull () { 
return type==type->vm.nullClass; 
}

bool ClassTypeInfo::isUndefined () { 
return type==type->vm.undefinedClass; 
}

bool ClassTypeInfo::equals (const std::shared_ptr<TypeInfo>& other) {
auto cti = dynamic_pointer_cast<ClassTypeInfo>(other);
if (!cti) return false;
return type==cti->type && exact==cti->exact && optional==cti->optional;
}

string ClassTypeInfo::toString () { 
string s = type->name.c_str(); 
if (exact) s+='!';
if (optional) s+= '?';
return s;
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
if (t0->isNullOrUndefined()) {
if (optional) return shared_from_this();
else return make_shared<ClassTypeInfo>(type, exact, true);
}
auto t = dynamic_pointer_cast<ClassTypeInfo>(t0);
if (!t) return t0->merge(shared_from_this(), ta);
if (t->type==type) {
if (exact==t->exact && optional==t->optional) return shared_from_this();
else return make_shared<ClassTypeInfo>(type, exact && t->exact, optional || t->optional);
}
QClass* cls = findCommonParent(type, t->type);
if (cls) return make_shared<ClassTypeInfo>(cls, false, optional || t->optional);
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

NamedTypeInfo::NamedTypeInfo (const QToken& t, bool ex, bool op):
token(t), exact(ex), optional(op) {}

string NamedTypeInfo::toBinString (QVM& vm) {
string s = format("Q%s;", string(token.start, token.length));
if (exact) s+='!';
if (optional) s+='?';
return s;
}

bool NamedTypeInfo::equals (const std::shared_ptr<TypeInfo>& other) { 
auto nti = dynamic_pointer_cast<NamedTypeInfo>(other);
if (!nti) return false;
return nti->token.length==token.length 
&& 0==strncmp(nti->token.start, token.start, token.length)
&& exact==nti->exact 
&& optional==nti->optional;
}

shared_ptr<TypeInfo> NamedTypeInfo::resolve (TypeAnalyzer& ta) {
AnalyzedVariable* lv = nullptr;
do {
lv = ta.findVariable(token, LV_EXISTING | LV_FOR_READ);
if (lv) break;
ClassDeclaration* cls = ta.getCurClass();
if (cls) {
auto m = cls->findMethod(token, false);
if (!m) m = cls->findMethod(token, true);
if (!m) break;
return m->returnType;
}
}while(false);
if (lv && lv->value) {
if (auto cd = dynamic_pointer_cast<ClassDeclaration>(lv->value)) {
return make_shared<ClassDeclTypeInfo>(cd, exact, optional);
}
else if (auto cst = dynamic_pointer_cast<ConstantExpression>(lv->value)) {
auto value = cst->token.value;
if (value.isInstanceOf(ta.vm.classClass)) {
QClass* cls = value.asObject<QClass>();
return make_shared<ClassTypeInfo>(cls, exact, optional);
}}
}//if lv->value
println("CAn't resolve '%s', lv=%s, value=%s", string(token.start, token.length), !!lv, lv&&lv->value?typeid(*lv->value).name() : "<null>");
return TypeInfo::MANY;
}

ClassDeclTypeInfo::ClassDeclTypeInfo (ClassDeclaration* c1, bool ex, bool op):
cls(c1), exact(ex), optional(op) {
}

ClassDeclTypeInfo::ClassDeclTypeInfo (shared_ptr<ClassDeclaration> c1, bool ex, bool op):
ClassDeclTypeInfo(c1.get(), ex, op) {}

bool ClassDeclTypeInfo::equals (const std::shared_ptr<TypeInfo>& other) {
auto cdti = dynamic_pointer_cast<ClassDeclTypeInfo>(other);
if (!cdti) return false;
return cls==cdti->cls && exact==cdti->exact && optional==cdti->optional;
}

shared_ptr<TypeInfo> ClassDeclTypeInfo::merge (shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) { 
if (!t0 || t0->isEmpty()) return shared_from_this();
t0 = t0->resolve(ta);
if (t0->isNullOrUndefined()) {
if (optional) return shared_from_this();
else return make_shared<ClassDeclTypeInfo>(cls, exact, true);
}
auto t = dynamic_pointer_cast<ClassDeclTypeInfo>(t0);
if (t && t->cls==cls) {
if (exact==t->exact && optional==t->optional) return shared_from_this();
else return make_shared<ClassDeclTypeInfo>(cls, exact && t->exact, optional || t->optional);
}
//println("Merging %s  and %s (%s)", toString(), t0->toString(), typeid(*t0).name());
shared_ptr<TypeInfo> re = TypeInfo::MANY;
vector<shared_ptr<TypeInfo>> v1 = { shared_from_this() };
for (auto& p: cls->parents) v1.push_back(make_shared<NamedTypeInfo>(p)->resolve(ta));
if (t) {
vector<shared_ptr<TypeInfo>> v2 = { t };
for (auto& p: t->cls->parents) v2.push_back(make_shared<NamedTypeInfo>(p)->resolve(ta));
for (auto a0: v1) {
for (auto b0: v2) {
if (b0==t && a0==shared_from_this()) continue;
auto a = dynamic_pointer_cast<ClassDeclTypeInfo>(a0);
auto b = dynamic_pointer_cast<ClassDeclTypeInfo>(b0);
if (a && b && a->cls==b->cls) { 
re = make_shared<ClassDeclTypeInfo>(a->cls, false, optional || t->optional);
goto found;
}}}
for (auto a0: v1) {
for (auto b0: v2) {
if (b0==t && a0==shared_from_this()) continue;
//auto a = dynamic_pointer_cast<ClassDeclTypeInfo>(a0);
//auto b = dynamic_pointer_cast<ClassDeclTypeInfo>(b0);
//if (a&&b) continue;
re = ta.mergeTypes(a0, b0);
if (re!=TypeInfo::MANY) goto found;
}}
}//if t
v1.erase(v1.begin());
for (auto t1: v1) {
re = ta.mergeTypes(t0, t1);
if (re!=TypeInfo::MANY) goto found;
}
found:
//println("Merged %s  and %s (%s) into %s (%s)", toString(), t0->toString(), typeid(*t0).name(), re->toString(), typeid(*re).name());
return re;
}

string ClassDeclTypeInfo::toString () { 
string s(cls->name.start, cls->name.length); 
if (exact) s+='!';
if (optional) s+='?';
return s;
}

string ClassDeclTypeInfo::toBinString (QVM& vm) {
string s = format("Q%s;", string(cls->name.start, cls->name.length));
if (exact) s+='!';
if (optional) s+='?';
return s;
}


ComposedTypeInfo::ComposedTypeInfo (shared_ptr<TypeInfo> tp, const vector<shared_ptr<TypeInfo>>& st):
type(tp), subtypes(st) {}

std::shared_ptr<TypeInfo> ComposedTypeInfo::resolve (TypeAnalyzer& ta) {
type = type? type->resolve(ta) :nullptr;
for (int i=0, n=countSubtypes(); i<n; i++) subtypes[i] = subtypes[i]? subtypes[i]->resolve(ta) :nullptr;
return shared_from_this();
}

shared_ptr<TypeInfo> ComposedTypeInfo::merge (shared_ptr<TypeInfo> t0, TypeAnalyzer&  ta) {
if (!t0 || t0->isEmpty()) return shared_from_this();
t0 = t0->resolve(ta);
auto t = dynamic_pointer_cast<ComposedTypeInfo>(t0);
if (!t) return t0->merge(type, ta);
else if (t->countSubtypes()!=countSubtypes()) return t->type->merge(type, ta);
auto newType = type->merge(t->type, ta);
vector<shared_ptr<TypeInfo>> newSubtypes;
newSubtypes.resize(countSubtypes());
for (int i=0, n=countSubtypes(); i<n; i++) newSubtypes[i] = ta.mergeTypes(subtypes[i], t->subtypes[i]);
return make_shared<ComposedTypeInfo>(newType, newSubtypes);
}

string ComposedTypeInfo::toString () {
ostringstream out;
out << type->toString() << '<';
for (int i=0, n=countSubtypes(); i<n; i++) {
if (i>0) out << ',';
out << subtypes[i]->toString();
}
out << '>';
return out.str();
}

string ComposedTypeInfo::toBinString (QVM& vm) {
ostringstream out;
out << 'C';
out << type->toBinString(vm);
out << '<';
for (int i=0, n=countSubtypes(); i<n; i++) out << subtypes[i]->toBinString(vm);
out << '>';
return out.str();
}

bool ComposedTypeInfo::equals (const std::shared_ptr<TypeInfo>& other) { 
auto cti = dynamic_pointer_cast<ComposedTypeInfo>(other);
if (!cti) return false;
if (!type || !cti->type) return false;
if (countSubtypes()!=cti->countSubtypes()) return false;
if (!type->equals(cti->type)) return false;
for (int i=0, n=countSubtypes(); i<n; i++) {
if (!subtypes[i] || !cti->subtypes[i]) return false;
if (!subtypes[i]->equals(cti->subtypes[i])) return false;
}
return true;
}

