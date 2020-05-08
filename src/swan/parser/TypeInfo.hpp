#ifndef ___COMPILER_PARSER_TYPE_INFO
#define ___COMPILER_PARSER_TYPE_INFO
#include "StatementBase.hpp"
#include "Token.hpp"
#include<memory>
#include<string>

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
ClassTypeInfo (QClass* cls): type(cls) {}
virtual bool isNum (QVM& vm) override;
virtual bool isBool (QVM& vm) override;
virtual std::string toString () override;
virtual std::string toBinString (QVM& vm) override;
virtual bool equals (const std::shared_ptr<TypeInfo>& other) { if (auto ci = std::dynamic_pointer_cast<ClassTypeInfo>(other)) return ci->type==type; return false; }
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) override;
QClass* findCommonParent (QClass* t1, QClass* t2);
};

struct NamedTypeInfo: TypeInfo {
QToken token;
NamedTypeInfo (const QToken& t): token(t) {}
std::shared_ptr<TypeInfo> resolve (TypeAnalyzer& ta) override;
std::string toString () override { return std::string(token.start, token.length); }
virtual std::string toBinString (QVM& vm) override;
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, TypeAnalyzer& ta) override { return resolve(ta)->merge(t? t->resolve(ta) : t, ta); }
virtual bool equals (const std::shared_ptr<TypeInfo>& other) override;
};

struct ComposedTypeInfo: TypeInfo {
std::shared_ptr<TypeInfo> type;
std::unique_ptr<std::shared_ptr<TypeInfo>[]> subtypes;
int nSubtypes;

ComposedTypeInfo (std::shared_ptr<TypeInfo> tp, int nst, std::unique_ptr<std::shared_ptr<TypeInfo>[]>&& st);
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) override;
std::shared_ptr<TypeInfo> resolve (TypeAnalyzer& ta) override;
std::string toString () override;
std::string toBinString (QVM& vm) override;
bool equals (const std::shared_ptr<TypeInfo>& other) override;
};

struct ClassDeclTypeInfo: TypeInfo {
std::shared_ptr<struct ClassDeclaration> cls;
ClassDeclTypeInfo (std::shared_ptr<ClassDeclaration> c1): cls(c1) {}
ClassDeclTypeInfo (ClassDeclaration* c1);
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, TypeAnalyzer& ta) override;
std::string toString () override;
std::string toBinString (QVM& vm) override;
bool equals (const std::shared_ptr<TypeInfo>& other) override { if (auto cd = std::dynamic_pointer_cast<ClassDeclTypeInfo>(other)) return cd->cls==cls; return false; }
};


#endif
