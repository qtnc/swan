#ifndef ___COMPILER_PARSER_TYPE_INFO
#define ___COMPILER_PARSER_TYPE_INFO
#include "StatementBase.hpp"
#include "Token.hpp"
#include "../../include/bitfield.hpp"
#include<memory>
#include<string>
#include<vector>

struct QVM;
struct QClass;

bitfield(TypeInfoFlag, uint32_t){
None = 0,
Exact = 1,
Optional = 2,
Static = 4,
};

struct AnyTypeInfo: TypeInfo {
bool isEmpty () override { return true; }
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, TypeAnalyzer& ta) override { return t?t->resolve(ta) : shared_from_this(); }
virtual std::string toString () override { return "*"; }
virtual std::string toBinString (QVM& vm) override { return "*"; }
virtual bool equals (const std::shared_ptr<TypeInfo>& other) { return !!std::dynamic_pointer_cast<AnyTypeInfo>(other); }
};

struct ManyTypeInfo: TypeInfo {
virtual std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, TypeAnalyzer& ta) override { return shared_from_this(); }
virtual std::string toString () override { return "#"; }
virtual std::string toBinString (QVM& vm) override { return "#"; }
virtual bool equals (const std::shared_ptr<TypeInfo>& other) { return !!std::dynamic_pointer_cast<ManyTypeInfo>(other); }
};

struct SubindexTypeInfo: TypeInfo {
int index;
SubindexTypeInfo (int i): index(i) {}
virtual std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, TypeAnalyzer& ta) override { return shared_from_this(); }
std::string toString () override;
std::string toBinString (QVM& vm) override ;
virtual bool equals (const std::shared_ptr<TypeInfo>& other) { 
if (auto o = std::dynamic_pointer_cast<SubindexTypeInfo>(other)) return o->index==index;
else return false;
}};

struct ClassTypeInfo: TypeInfo {
QClass* type;
bitmask<TypeInfoFlag> flags;

ClassTypeInfo (QClass* cls, bitmask<TypeInfoFlag> flags = TypeInfoFlag::None);
bool isNum () override;
bool isBool () override;
bool isString () override;
bool isRange  () override;
bool isFunction () override;
bool isNull () override;
bool isUndefined () override;
bool isExact () override { return static_cast<bool>(flags & TypeInfoFlag::Exact); }
bool isOptional () override { return static_cast<bool>(flags & TypeInfoFlag::Optional); }
std::string toString () override;
std::string toBinString (QVM& vm) override;
bool equals (const std::shared_ptr<TypeInfo>& other) override;
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) override;
QClass* findCommonParent (QClass* t1, QClass* t2);
};

struct NamedTypeInfo: TypeInfo {
QToken token;
bitmask<TypeInfoFlag> flags;

NamedTypeInfo (const QToken& t, bitmask<TypeInfoFlag> flags = TypeInfoFlag::None);
std::shared_ptr<TypeInfo> resolve (TypeAnalyzer& ta) override;
std::string toString () override { return std::string(token.start, token.length); }
std::string toBinString (QVM& vm) override;
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, TypeAnalyzer& ta) override { return resolve(ta)->merge(t, ta); }
bool equals (const std::shared_ptr<TypeInfo>& other) override;
};

struct ComposedTypeInfo: TypeInfo {
std::shared_ptr<TypeInfo> type;
std::vector<std::shared_ptr<TypeInfo>> subtypes;

ComposedTypeInfo (std::shared_ptr<TypeInfo> tp, const std::vector<std::shared_ptr<TypeInfo>>& st);
int countSubtypes () const { return subtypes.size(); }
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) override;
std::shared_ptr<TypeInfo> resolve (TypeAnalyzer& ta) override;
std::string toString () override;
std::string toBinString (QVM& vm) override;
bool equals (const std::shared_ptr<TypeInfo>& other) override;
bool isExact () override { return type && type->isExact(); }
bool isOptional () override { return type && type->isOptional(); }
};

struct ClassDeclTypeInfo: TypeInfo {
struct ClassDeclaration* cls;
bitmask<TypeInfoFlag> flags;

ClassDeclTypeInfo (std::shared_ptr<ClassDeclaration> c1, bitmask<TypeInfoFlag> flags = TypeInfoFlag::None);
ClassDeclTypeInfo (ClassDeclaration* c1, bitmask<TypeInfoFlag> flags = TypeInfoFlag::None);
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) override;
std::string toString () override;
std::string toBinString (QVM& vm) override;
bool equals (const std::shared_ptr<TypeInfo>& other) override;
};

#endif
