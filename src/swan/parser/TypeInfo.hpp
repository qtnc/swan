#ifndef ___COMPILER_PARSER_TYPE_INFO
#define ___COMPILER_PARSER_TYPE_INFO
#include "StatementBase.hpp"
#include "Token.hpp"
#include<memory>
#include<string>
#include<vector>

struct QVM;
struct QClass;

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

struct ClassTypeInfo: TypeInfo {
QClass* type;
bool exact, optional;

ClassTypeInfo (QClass* cls, bool exact=false, bool optional=false);
bool isNum (QVM& vm) override;
bool isBool (QVM& vm) override;
bool isString (QVM& vm) override;
bool isNull (QVM& vm) override;
bool isUndefined (QVM& vm) override;
bool isExact () override { return exact; }
bool isOptional () override { return optional; }
std::string toString () override;
std::string toBinString (QVM& vm) override;
bool equals (const std::shared_ptr<TypeInfo>& other) override;
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) override;
QClass* findCommonParent (QClass* t1, QClass* t2);
};

struct NamedTypeInfo: TypeInfo {
QToken token;
bool exact, optional;

NamedTypeInfo (const QToken& t, bool exact=false, bool optional=false);
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
bool exact, optional;

ClassDeclTypeInfo (std::shared_ptr<ClassDeclaration> c1, bool exact=false, bool optional=false);
ClassDeclTypeInfo (ClassDeclaration* c1, bool exact=false, bool optional=false);
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) override;
std::string toString () override;
std::string toBinString (QVM& vm) override;
bool equals (const std::shared_ptr<TypeInfo>& other) override;
};

#endif
