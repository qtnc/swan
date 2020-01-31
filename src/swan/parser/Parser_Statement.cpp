#include "Parser.hpp"
#include "ParserRules.hpp"
#include "Statement.hpp"
#include "Expression.hpp"
#include "../vm/VM.hpp"
using namespace std;

void QParser::multiVarExprToSingleLiteralMap (vector<shared_ptr<Variable>>& vars, int flags) {
if (vars.size()==1 && dynamic_pointer_cast<LiteralMapExpression>(vars[0]->name)) return;
auto map = make_shared<LiteralMapExpression>(vars[0]->name->nearestToken()), defmap = make_shared<LiteralMapExpression>(vars[0]->name->nearestToken());
for (auto& var: vars) {
shared_ptr<Expression> key=nullptr, value=nullptr, name = nameExprToConstant(var->name);
if (var->value) {
key = BinaryOperation::create(name, T_EQ, var->value);
value = BinaryOperation::create(var->name, T_EQ, var->value);
}
else {
key = name;
value = var->name;
}
map->items.push_back(make_pair(key, value));
}
vars.clear();
vars.push_back(make_shared<Variable>(map, defmap, flags));
}

shared_ptr<Statement> QParser::parseSimpleStatement () {
return make_shared<SimpleStatement>(cur);
}

shared_ptr<Statement> QParser::parseBlock () {
vector<shared_ptr<Statement>> statements;
while(!match(T_RIGHT_BRACE)) {
shared_ptr<Statement> sta = parseStatement();
if (sta) statements.push_back(sta);
else { result=CR_INCOMPLETE; break; }
skipNewlines();
}
if (!statements.size()) statements.push_back(make_shared<SimpleStatement>(cur));
return make_shared<BlockStatement>(statements);
}

shared_ptr<Statement> QParser::parseIf () {
shared_ptr<Expression> condition = parseExpression();
match(T_COLON);
shared_ptr<Statement> ifPart = parseStatement();
if (!ifPart) result = CR_INCOMPLETE;
shared_ptr<Statement> elsePart = nullptr;
skipNewlines();
if (match(T_ELSE)) {
match(T_COLON);
elsePart = parseStatement();
if (!elsePart) result = CR_INCOMPLETE;
}
return make_shared<IfStatement>(condition, ifPart, elsePart);
}

shared_ptr<Statement> QParser::parseSwitchStatement () {
auto sw = make_shared<SwitchStatement>();
shared_ptr<Expression> activeCase;
vector<shared_ptr<Statement>> statements;
bool defaultDefined=false;
auto clearStatements = [&]()mutable{
if (activeCase) sw->cases.push_back(make_pair(activeCase, statements));
else sw->defaultCase = statements;
statements.clear();
};
sw->expr = parseExpression();
sw->var = make_shared<NameExpression>(createTempName());
skipNewlines();
consume(T_LEFT_BRACE, "Expected '{' to begin switch");
while(true){
skipNewlines();
if (match(T_CASE)) {
if (defaultDefined) parseError("Default case must be last");
clearStatements();
activeCase = parseSwitchCase(sw->var);
while(match(T_COMMA)) {
clearStatements();
activeCase = parseSwitchCase(sw->var);
}
match(T_COLON);
}
else if (match(T_DEFAULT)) {
if (defaultDefined) parseError("Duplicate default case");
defaultDefined=true;
clearStatements();
activeCase = nullptr;
match(T_COLON);
}
else if (match(T_RIGHT_BRACE)) break;
else {
auto sta = parseStatement();
if (!sta) { result=CR_INCOMPLETE; return nullptr; }
statements.push_back(sta);
}}
clearStatements();
return sw;
}

void ForStatement::parseHead (QParser& parser) {
parser.parseVarList(loopVariables);
if (loopVariables.size()==1 && (loopVariables[0]->flags&VD_NODEFAULT) && parser.match(T_IN)) {
parser.skipNewlines();
inExpression = parser.parseExpression(P_COMPREHENSION);
} else {
parser.consume(T_SEMICOLON, "Expected ';' after traditional for loop variables declarations");
traditional=true;
inExpression = parser.parseExpression();
parser.consume(T_SEMICOLON, "Expected ';' after traditional for loop condition");
incrExpression = parser.parseExpression();
}}

shared_ptr<Statement> QParser::parseFor () {
shared_ptr<ForStatement> forSta = make_shared<ForStatement>(cur);
forSta->parseHead(*this);
match(T_COLON);
forSta->loopStatement = parseStatement();
if (!forSta->inExpression || !forSta->loopStatement) result = CR_INCOMPLETE;
return forSta;
}

shared_ptr<Statement> QParser::parseWhile () {
shared_ptr<Expression> condition = parseExpression();
match(T_COLON);
shared_ptr<Statement> loopStatement = parseStatement();
if (!loopStatement) result = CR_INCOMPLETE;
return make_shared<WhileStatement>(condition, loopStatement);
}

shared_ptr<Statement> QParser::parseRepeatWhile () {
shared_ptr<Statement> loopStatement = parseStatement();
consume(T_WHILE, ("Expected 'while' after repeated statement"));
shared_ptr<Expression> condition = parseExpression();
if (!loopStatement || !condition) result = CR_INCOMPLETE;
return make_shared<RepeatWhileStatement>(condition, loopStatement);
}

shared_ptr<Statement> QParser::parseContinue () {
QToken cs = cur;
int count = 1;
if (match(T_NUM)) count = cur.value.d;
return make_shared<ContinueStatement>(cs, count);
}

shared_ptr<Statement> QParser::parseBreak () {
QToken bks = cur;
int count = 1;
if (match(T_NUM)) count = cur.value.d;
return make_shared<BreakStatement>(bks, count);
}

shared_ptr<Statement> QParser::parseReturn () {
QToken returnToken = cur;
shared_ptr<Expression> expr = nullptr;
if (matchOneOf(T_RIGHT_BRACE, T_LINE, T_SEMICOLON)) prevToken();
else expr = parseExpression();
return make_shared<ReturnStatement>(returnToken, expr);
}

shared_ptr<Statement> QParser::parseThrow () {
QToken tk = cur;
return make_shared<ThrowStatement>(tk, parseExpression());
}

shared_ptr<Statement> QParser::parseTry () {
QToken catchVar = cur;
match(T_COLON);
shared_ptr<Statement> tryPart = parseStatement();
if (!tryPart) result = CR_INCOMPLETE;
shared_ptr<Statement> catchPart = nullptr, finallyPart = nullptr;
skipNewlines();
if (match(T_CATCH)) {
bool paren = match(T_LEFT_PAREN);
consume(T_NAME, "Expected variable name after 'catch'");
catchVar = cur;
if (paren) consume(T_RIGHT_PAREN, "Expected ')' after catch variable name");
match(T_COLON);
catchPart = parseStatement();
if (!catchPart) result = CR_INCOMPLETE;
}
skipNewlines();
if (match(T_FINALLY)) {
match(T_COLON);
finallyPart = parseStatement();
if (!finallyPart) result = CR_INCOMPLETE;
}
if (!catchPart && !finallyPart) result = CR_INCOMPLETE;
return make_shared<TryStatement>(tryPart, catchPart, finallyPart, catchVar);
}

shared_ptr<Statement> QParser::parseWith () {
auto withSta = make_shared<WithStatement>();
skipNewlines();
withSta->parseHead(*this);
skipNewlines();
match(T_COLON);
withSta->body = parseStatement();
skipNewlines();
withSta->parseTail(*this);
return withSta;
}

void WithStatement::parseHead (QParser& parser) {
auto expr = parser.parseExpression(P_COMPREHENSION);
if (parser.match(T_EQ)) {
varExpr = expr;
openExpr = parser.parseExpression(P_COMPREHENSION);
}
else {
varExpr = make_shared<NameExpression>(parser.createTempName());
openExpr = expr;
}
if (!dynamic_pointer_cast<NameExpression>(varExpr)) {
varExpr = openExpr = nullptr;
parser.parseError("Invalid variable name for 'with' expression");
}
if (!varExpr || !openExpr) parser.result=CR_INCOMPLETE; 
}

void WithStatement::parseTail (QParser& parser) {
if (parser.match(T_CATCH)) {
bool paren = parser.match(T_LEFT_PAREN);
parser.consume(T_NAME, "Expected variable name after 'catch'");
catchVar = parser.cur;
if (paren) parser.consume(T_RIGHT_PAREN, "Expected ')' after catch variable name");
parser.match(T_COLON);
catchPart = parser.parseStatement();
if (!catchPart) parser.result=CR_INCOMPLETE;
}}

shared_ptr<Statement> QParser::parseDecoratedStatement () {
auto decoration = parseExpression(P_PREFIX);
auto expr = parseStatement();
if (expr && expr->isDecorable()) {
auto decorable = dynamic_pointer_cast<Decorable>(expr);
decorable->decorations.insert(decorable->decorations.begin(), decoration);
}
else parseError("Expression can't be decorated");
return expr;
}


shared_ptr<Statement> QParser::parseAsync () {
return parseAsyncFunctionDecl(VD_CONST);
}

shared_ptr<Statement> QParser::parseAsyncFunctionDecl (int varFlags) {
consume(T_FUNCTION, "Expected 'function' after 'async'");
return parseFunctionDecl(varFlags, FD_ASYNC);
}

shared_ptr<Statement> QParser::parseFunctionDecl () {
return parseFunctionDecl(0);
}

shared_ptr<Statement> QParser::parseFunctionDecl (int varFlags, int funcFlags) {
bool hasName = matchOneOf(T_NAME, T_STRING);
QToken name = cur;
auto fnDecl = parseLambda(funcFlags);
if (!hasName) return fnDecl;
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) varFlags |= VD_GLOBAL;
vector<shared_ptr<Variable>> vars = { make_shared<Variable>( make_shared<NameExpression>(name), fnDecl, varFlags) };
return make_shared<VariableDeclaration>(vars);
}

shared_ptr<Statement> QParser::parseClassDecl () {
return parseClassDecl(0);
}

shared_ptr<Statement> QParser::parseClassDecl (int classFlags) {
if (!consume(T_NAME, ("Expected class name after 'class'"))) return nullptr;
shared_ptr<ClassDeclaration> classDecl = make_shared<ClassDeclaration>(cur, classFlags);
skipNewlines();
if (matchOneOf(T_IS, T_COLON, T_LT)) do {
skipNewlines();
consume(T_NAME, ("Expected class name after 'is'"));
classDecl->parents.push_back(cur);
} while (match(T_COMMA));
else classDecl->parents.push_back({ T_NAME, ("Object"), 6, QV::UNDEFINED });
if (match(T_LEFT_BRACE)) {
while(true) {
skipNewlines();
int memberFlags = 0;
if (nextToken().type==T_STATIC) {
memberFlags |= FD_STATIC;
nextToken();
}
const ParserRule& rule = rules[cur.type];
if (rule.member) (this->*rule.member)(*classDecl, memberFlags);
else { prevToken(); break; }
}
skipNewlines();
consume(T_RIGHT_BRACE, ("Expected '}' to close class body"));
}
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) classFlags |= VD_GLOBAL;
vector<shared_ptr<Variable>> vars = { make_shared<Variable>( make_shared<NameExpression>(classDecl->name), classDecl, classFlags) };
return make_shared<VariableDeclaration>(vars);
}

shared_ptr<Statement> QParser::parseGlobalDecl () {
if (matchOneOf(T_VAR, T_CONST)) {
return parseVarDecl(VD_GLOBAL);
}
else if (match(T_CLASS)) {
return parseClassDecl(VD_CONST | VD_GLOBAL);
}
else if (match(T_FUNCTION)) {
return parseFunctionDecl(VD_CONST | VD_GLOBAL);
}
else if (match(T_ASYNC)) {
return parseAsyncFunctionDecl(VD_CONST | VD_GLOBAL);
}
parseError(("Expected 'function', 'var' or 'class' after 'global'"));
return nullptr;
}

shared_ptr<Statement> QParser::parseExportDecl () {
auto exportDecl = make_shared<ExportDeclaration>();
shared_ptr<VariableDeclaration> varDecl;
if (matchOneOf(T_VAR, T_CONST)) varDecl = dynamic_pointer_cast<VariableDeclaration>(parseVarDecl(VD_CONST));
else if (match(T_CLASS)) varDecl = dynamic_pointer_cast<VariableDeclaration>(parseClassDecl(VD_CONST));
else if (match(T_FUNCTION)) varDecl = dynamic_pointer_cast<VariableDeclaration>(parseFunctionDecl(VD_CONST));
else if (match(T_ASYNC)) varDecl = dynamic_pointer_cast<VariableDeclaration>(parseAsyncFunctionDecl(VD_CONST));
if (varDecl) {
auto name = varDecl->vars[0]->name;
exportDecl->exports.push_back(make_pair(name->nearestToken(), name));
vector<shared_ptr<Statement>> sta = { varDecl, exportDecl };
return make_shared<BlockStatement>(sta, false);
}
else {
do {
shared_ptr<Expression> exportExpr = parseExpression();
if (!exportExpr) { parseError("Expected 'class', 'function', 'var' or expression after 'export'"); return nullptr; }
QToken nameToken = cur;
shared_ptr<NameExpression> nameExpr = dynamic_pointer_cast<NameExpression>(exportExpr);
if (nameExpr && !match(T_AS)) nameToken = nameExpr->token;
else {
if (!nameExpr) consume(T_AS, "Expected 'as' after export expression");
consume(T_NAME, "Expected export name after 'as'");
nameToken = cur;
}
exportDecl->exports.push_back(make_pair(nameToken, exportExpr));
} while (match(T_COMMA));
}
return exportDecl;
}


shared_ptr<Statement> QParser::parseImportDecl () {
auto importSta = make_shared<ImportDeclaration>();
int flags = 0;
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VD_GLOBAL;
parseVarList(importSta->imports, flags);
multiVarExprToSingleLiteralMap(importSta->imports, flags);
consume(T_IN, "Expected 'in' after import variables");
importSta->from = parseExpression(P_COMPREHENSION);
return importSta;
}

shared_ptr<Statement> QParser::parseImportDecl2 () {
auto importSta = make_shared<ImportDeclaration>();
int flags = 0;
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VD_GLOBAL;
importSta->from = parseExpression(P_COMPREHENSION);
consume(T_IMPORT, "Expected 'import' after import source");
parseVarList(importSta->imports, flags);
multiVarExprToSingleLiteralMap(importSta->imports, flags);
return importSta;
}

shared_ptr<Statement> QParser::parseStatement () {
skipNewlines();
const ParserRule& rule = rules[nextToken().type];
if (rule.statement) return (this->*rule.statement)();
else if (cur.type==T_END) return nullptr;
else {
prevToken();
return parseExpression();
}}

shared_ptr<Statement> QParser::parseStatements () {
vector<shared_ptr<Statement>> statements;
while(!matchOneOf(T_END, T_RIGHT_BRACE)) {
shared_ptr<Statement> sta = parseStatement();
if (sta) statements.push_back(sta);
else break;
}
return make_shared<BlockStatement>(statements);
}