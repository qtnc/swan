#ifndef _____TYPE_ANALYZER_HPP_____
#define _____TYPE_ANALYZER_HPP_____
#include "Constants.hpp"
#include "Token.hpp"
#include "Parser.hpp"
#include "../vm/Function.hpp"
#include "../../include/cpprintf.hpp"
#include<string>
#include<vector>

struct QVM;
struct QParser;
struct FuncOrDecl;

struct AnalyzedVariable {
QToken name;
std::shared_ptr<struct TypeInfo> type;
std::shared_ptr<struct Expression> value;
int scope;
AnalyzedVariable (const QToken& n, int s);
};

struct TypeAnalyzer {
QVM& vm;
QParser& parser;
struct ClassDeclaration* curClass = nullptr;
struct FunctionDeclaration* curMethod = nullptr;
TypeAnalyzer* parent = nullptr;
std::vector<AnalyzedVariable> variables;
int curScope = 0;

void pushScope ();
void popScope ();

AnalyzedVariable* findVariable (const QToken& name, int flags);

struct ClassDeclaration* getCurClass (int* atLevel = nullptr);
struct FunctionDeclaration* getCurMethod ();

std::shared_ptr<TypeInfo> mergeTypes (std::shared_ptr<TypeInfo> t1, std::shared_ptr<TypeInfo> t2);
std::shared_ptr<TypeInfo> resolveCallType  (std::shared_ptr<Expression> receiver, const QToken& methodName, int nArgs=0, std::shared_ptr<Expression>* args = nullptr, bool super = false, FuncOrDecl* fd = nullptr);
std::shared_ptr<TypeInfo> resolveCallType  (std::shared_ptr<Expression> receiver, struct ClassDeclaration&  cls, const QToken& methodName, int nArgs=0, std::shared_ptr<Expression>* args = nullptr, bool super = false, FuncOrDecl* fd = nullptr);
std::shared_ptr<TypeInfo> resolveCallType  (std::shared_ptr<Expression> receiver, QV func, int nArgs, std::shared_ptr<Expression>* args, FuncOrDecl* fd =nullptr);
std::shared_ptr<TypeInfo> resolveCallType  (std::shared_ptr<Expression> func, int nArgs, std::shared_ptr<Expression>* args, FuncOrDecl* fd = nullptr);
std::shared_ptr<TypeInfo> resolveValueType (QV value);
bool isSameType (const std::shared_ptr<TypeInfo>& t1, const std::shared_ptr<TypeInfo>& t2);
int assignType (Expression& e, const std::shared_ptr<TypeInfo>& type);

inline QToken createTempName (Expression& expr) { return parser.createTempName(expr); }
inline QToken createTempName (const QToken& expr) { return parser.createTempName(expr); }

template<class... A> inline void typeError (const QToken& token, const char* fmt, const A&... args) { parser.printMessage( token, Swan::CompilationMessage::Kind::ERROR, format(fmt, args...)); }
template<class... A> inline void typeWarn (const QToken& token, const char* fmt, const A&... args) { parser.printMessage( token, Swan::CompilationMessage::Kind::WARNING, format(fmt, args...)); }
template<class... A> inline void typeInfo (const QToken& token, const char* fmt, const A&... args) { parser.printMessage( token, Swan::CompilationMessage::Kind::INFO, format(fmt, args...)); }

TypeAnalyzer (QParser& p, TypeAnalyzer* pa = nullptr): vm(p.vm), parser(p), parent(pa), curClass(nullptr)  {}
void report  (std::shared_ptr<Expression> expr);
};

#endif
