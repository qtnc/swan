#ifndef ___PARRSER_COMPILER_STATEMENT1
#define ___PARRSER_COMPILER_STATEMENT1
#include "StatementBase.hpp"
#include "../vm/OpCodeInfo.hpp"

struct SimpleStatement: Statement {
QToken token;
SimpleStatement (const QToken& t): token(t) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override {}
int analyze (TypeAnalyzer& ta) override { return false; }
bool isPure () override { return true; }
};

struct IfStatement: Statement, Comprenable   {
std::shared_ptr<Expression> condition;
std::shared_ptr<Statement> ifPart, elsePart;
QOpCode jumpType;
IfStatement (std::shared_ptr<Expression> cond, std::shared_ptr<Statement> ifp=nullptr, std::shared_ptr<Statement> ep = nullptr, QOpCode jt = OP_JUMP_IF_FALSY): condition(cond), ifPart(ifp), elsePart(ep), jumpType(jt) {}
void chain (const std::shared_ptr<Statement>& st) final override;
std::shared_ptr<Statement> optimizeStatement () override;
const QToken& nearestToken () override { return condition->nearestToken(); }
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override;
bool isPure () override { return (!condition || condition->isPure()) && (!ifPart || ifPart->isPure()) && (!elsePart || elsePart->isPure()); }
};

struct SwitchStatement: Statement {
std::shared_ptr<Expression> expr, var;
std::vector<std::pair<std::shared_ptr<Expression>, std::vector<std::shared_ptr<Statement>>>> cases;
std::vector<std::shared_ptr<Statement>> defaultCase;
std::shared_ptr<Statement> optimizeStatement () override;
const QToken& nearestToken () override { return expr->nearestToken(); }
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override;
};

struct ForStatement: Statement, Comprenable  {
QToken token;
std::vector<std::shared_ptr<Variable>> loopVariables;
std::shared_ptr<Expression> inExpression, incrExpression;
std::shared_ptr<Statement> loopStatement;
bool traditional;
ForStatement (const QToken& tk): token(tk), loopVariables(), inExpression(nullptr), loopStatement(nullptr), incrExpression(nullptr), traditional(false)   {}
void chain (const std::shared_ptr<Statement>& st) final override;
std::shared_ptr<Statement> optimizeStatement () override;
const QToken& nearestToken () override { return token; }
void parseHead (struct QParser& parser);
void compile (QCompiler& compiler)override ;
void compileForEach (QCompiler& compiler);
void compileTraditional (QCompiler& compiler);
int analyze (TypeAnalyzer& ta) override;
int analyzeForEach (TypeAnalyzer& ta);
int analyzeTraditional (TypeAnalyzer& ta);
bool isPure () override { return (!inExpression || inExpression->isPure()) && (!incrExpression || incrExpression->isPure()) && (!loopStatement || loopStatement->isPure()); }
};

struct WhileStatement: Statement {
std::shared_ptr<Expression> condition;
std::shared_ptr<Statement> loopStatement;
WhileStatement (std::shared_ptr<Expression> cond, std::shared_ptr<Statement> lst): condition(cond), loopStatement(lst) {}
std::shared_ptr<Statement> optimizeStatement () override;
const QToken& nearestToken () override { return condition->nearestToken(); }
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override;
bool isPure () override { return (!condition || condition->isPure()) && (!loopStatement || loopStatement->isPure()); }
};

struct RepeatWhileStatement: Statement {
std::shared_ptr<Expression> condition;
std::shared_ptr<Statement> loopStatement;
RepeatWhileStatement (std::shared_ptr<Expression> cond, std::shared_ptr<Statement> lst): condition(cond), loopStatement(lst) {}
std::shared_ptr<Statement> optimizeStatement () override ;
const QToken& nearestToken () override { return loopStatement->nearestToken(); }
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override;
bool isPure () override { return (!condition || condition->isPure()) && (!loopStatement || loopStatement->isPure()); }
};

struct ContinueStatement: SimpleStatement {
int count;
ContinueStatement (const QToken& tk, int n = 1): SimpleStatement(tk), count(n) {}
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override { return false; }
};

struct BreakStatement: SimpleStatement {
int count;
BreakStatement (const QToken& tk, int n=1): SimpleStatement(tk), count(n) {}
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override { return false; }
};

struct ReturnStatement: Statement {
QToken returnToken;
std::shared_ptr<Expression> expr;
ReturnStatement (const QToken& retk, std::shared_ptr<Expression> e0 = nullptr): returnToken(retk), expr(e0) {}
const QToken& nearestToken () override { return expr? expr->nearestToken() : returnToken; }
std::shared_ptr<Statement> optimizeStatement () override;
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override;
};

struct ThrowStatement: Statement {
QToken returnToken;
std::shared_ptr<Expression> expr;
ThrowStatement (const QToken& retk, std::shared_ptr<Expression> e0): returnToken(retk), expr(e0) {}
const QToken& nearestToken () override { return expr? expr->nearestToken() : returnToken; }
std::shared_ptr<Statement> optimizeStatement () override;
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override { return false; }
};

struct TryStatement: Statement, Comprenable  {
std::shared_ptr<Statement> tryPart, catchPart, finallyPart;
QToken catchVar;
TryStatement (std::shared_ptr<Statement> tp, std::shared_ptr<Statement> cp, std::shared_ptr<Statement> fp, const QToken& cv): tryPart(tp), catchPart(cp), finallyPart(fp), catchVar(cv)  {}
const QToken& nearestToken () override { return tryPart->nearestToken(); }
void chain (const std::shared_ptr<Statement>& st) final override ;
std::shared_ptr<Statement> optimizeStatement () override;
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override;
bool isPure () override { return (!tryPart || tryPart->isPure()) && (!finallyPart || finallyPart->isPure()); }
};

struct WithStatement: Statement, Comprenable  {
std::shared_ptr<Expression> varExpr, openExpr;
std::shared_ptr<Statement> body, catchPart;
QToken catchVar;
WithStatement (): varExpr(nullptr), openExpr(nullptr), body(nullptr), catchPart(nullptr), catchVar({ T_END, nullptr, 0, QV::UNDEFINED }) {}
const QToken& nearestToken () override { return varExpr->nearestToken(); }
void chain (const std::shared_ptr<Statement>& st) final override ;
std::shared_ptr<Statement> optimizeStatement () override;
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override;
void parseHead (QParser& parser);
void parseTail  (QParser& parser);
bool isPure () override { return !body || body->isPure(); }
};

struct BlockStatement: Statement, Comprenable  {
std::vector<std::shared_ptr<Statement>> statements;
bool makeScope, optimized;
BlockStatement (const std::vector<std::shared_ptr<Statement>>& sta = {}, bool s = true): statements(sta), makeScope(s), optimized(false)   {}
void chain (const std::shared_ptr<Statement>& st) final override;
std::shared_ptr<Statement> optimizeStatement () override;
void doHoisting ();
const QToken& nearestToken () override { return statements[0]->nearestToken(); }
bool isUsingExports () override;
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override;
bool isPure () override { return std::all_of(statements.begin(), statements.end(), [&](auto& s){ return s->isPure(); }); }
};

struct VariableDeclaration: Statement, Decorable {
std::vector<std::shared_ptr<Variable>> vars;
VariableDeclaration (const std::vector<std::shared_ptr<Variable>>& v = {}): vars(v) {}
const QToken& nearestToken () override { return vars[0]->name->nearestToken(); }
bool isDecorable () override { return true; }
std::shared_ptr<Statement> optimizeStatement () override;
void compile (QCompiler& compiler)override ;
int analyze (TypeAnalyzer& ta) override;
};

struct ExportDeclaration: Statement  {
std::vector<std::pair<QToken,std::shared_ptr<Expression>>> exports;
const QToken& nearestToken () override { return exports[0].first; }
std::shared_ptr<Statement> optimizeStatement () override;
bool isUsingExports () override { return true; }
void compile (QCompiler& compiler)override ;
int analyze (TypeAnalyzer& ta) override { return false; }
};

struct ImportDeclaration: Statement {
std::shared_ptr<Expression> from;
std::vector<std::shared_ptr<Variable>> imports;
bool importAll = false;
std::shared_ptr<Statement> optimizeStatement () override;
const QToken& nearestToken () override { return from->nearestToken(); }
void compile (QCompiler& compiler) override;
int analyze (TypeAnalyzer& ta) override;
};


#endif
