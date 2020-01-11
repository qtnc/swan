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
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, QCompiler& compiler) override { return t?t->resolve(compiler) : shared_from_this(); }
virtual std::string toString () override { return "*"; }
};

struct ManyTypeInfo: TypeInfo {
virtual std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, QCompiler& compiler) override { return shared_from_this(); }
virtual std::string toString () override { return "#"; }
};

struct ClassTypeInfo: TypeInfo {
QClass* type;
ClassTypeInfo (QClass* cls): type(cls) {}
virtual bool isNum (QVM& vm) override;
virtual bool isBool (QVM& vm) override;
virtual std::string toString () override;
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, QCompiler& compiler) override;
QClass* findCommonParent (QClass* t1, QClass* t2);
};

struct NamedTypeInfo: TypeInfo {
QToken token;
NamedTypeInfo (const QToken& t): token(t) {}
std::shared_ptr<TypeInfo> resolve (QCompiler& compiler) override;
std::string toString () override { return std::string(token.start, token.length); }
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, QCompiler& compiler) override { return resolve(compiler)->merge(t? t->resolve(compiler) : t, compiler); }
};

struct ComposedTypeInfo: TypeInfo {
std::shared_ptr<TypeInfo> type;
std::unique_ptr<std::shared_ptr<TypeInfo>[]> subtypes;
int nSubtypes;

ComposedTypeInfo (std::shared_ptr<TypeInfo> tp, int nst, std::unique_ptr<std::shared_ptr<TypeInfo>[]>&& st);
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, QCompiler& compiler) override;
std::string toString () override;
};

struct ClassDeclTypeInfo: TypeInfo {
std::shared_ptr<struct ClassDeclaration> cls;
ClassDeclTypeInfo (std::shared_ptr<ClassDeclaration> c1): cls(c1) {}
ClassDeclTypeInfo (ClassDeclaration* c1);
std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t0, QCompiler& compiler) override;
std::string toString () override;
};


#endif
