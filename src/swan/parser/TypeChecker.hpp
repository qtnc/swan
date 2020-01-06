#ifndef _____TYPECHECKER_HPP
#define _____TYPECHECKER_HPP
#include "Constants.hpp"
#include "Token.hpp"
#include "Parser.hpp"
#include "../vm/OpCodeInfo.hpp"
#include "../vm/Function.hpp"
#include "../../include/cpprintf.hpp"
#include<string>
#include<vector>
#include<sstream>
#include<limits>

struct QVM;
struct QParser;

struct TypedVariable {
QToken name;
std::shared_ptr<struct TypeInfo> type;
int scope;
struct ClassDeclaration* classDecl;
struct FunctionDeclaration* funcDecl;
TypedVariable (const QToken& n, int s);
};

struct TypeChecker {
QVM& vm;
QParser& parser;
struct ClassDeclaration* curClass = nullptr;
struct FunctionDeclaration* curMethod = nullptr;
TypeChecker* parent = nullptr;
std::vector<TypedVariable> variables;
int curScope = 0;

void pushScope ();
void popScope ();

TypedVariable* findVariable (const QToken& name, int flags);
struct ClassDeclaration* getCurClass (int* atLevel = nullptr);
struct FunctionDeclaration* getCurMethod ();

std::shared_ptr<TypeInfo> resolveCallType (std::shared_ptr<Expression> receiver, const QToken& methodName, int nArgs=0, std::shared_ptr<Expression>* args = nullptr, bool super = false);
std::shared_ptr<TypeInfo> resolveCallType (std::shared_ptr<Expression> receiver, QV func, int nArgs, std::shared_ptr<Expression>* args);
std::shared_ptr<TypeInfo> getType (QV value);

TypeChecker (QParser& p, TypeChecker* p1 = nullptr): vm(p.vm), parser(p), parent(p1), curClass(nullptr)  {}
};

#endif
