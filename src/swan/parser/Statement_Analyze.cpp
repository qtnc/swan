#include "Statement.hpp"
#include "Expression.hpp"
#include "TypeAnalyzer.hpp"
#include "ParserRules.hpp"
#include "../vm/VM.hpp"
#include "../vm/Map.hpp"
using namespace std;

vector<shared_ptr<NameExpression>>& decompose (TypeAnalyzer& ta, shared_ptr<Expression> expr, vector<shared_ptr<NameExpression>>& names) {
if (auto name = dynamic_pointer_cast<NameExpression>(expr)) {
names.push_back(name);
return names;
}
if (auto seq = dynamic_pointer_cast<LiteralSequenceExpression>(expr)) {
for (auto& item: seq->items) decompose(ta, item, names);
return names;
}
if (auto map = dynamic_pointer_cast<LiteralMapExpression>(expr)) {
for (auto& item: map->items) decompose(ta, item.second, names);
return names;
}
{
auto bop = dynamic_pointer_cast<BinaryOperation>(expr);
if (bop && bop->op==T_EQ) {
decompose(ta, bop->left, names);
return names;
}}
if (auto th = dynamic_pointer_cast<TypeHintExpression>(expr)) {
decompose(ta, th->expr, names);
return names;
}
if (auto prop = dynamic_pointer_cast<GenericMethodSymbolExpression>(expr)) {
names.push_back(make_shared<NameExpression>(prop->token));
return names;
}
if (auto unpack = dynamic_pointer_cast<UnpackExpression>(expr)) {
decompose(ta, unpack->expr, names);
return names;
}
//if (!dynamic_cast<FieldExpression*>(&*expr) && !dynamic_cast<StaticFieldExpression*>(&*expr)) ta.compileError(expr->nearestToken(), "Invalid target for assignment in destructuring");
return names;
}

int IfStatement::analyze (TypeAnalyzer& ta) {
int re = condition->analyze(ta);
ta.pushScope();
re |= ifPart->analyze(ta);
ta.popScope();
if (elsePart) {
ta.pushScope();
re |= elsePart->analyze(ta);
ta.popScope();
}
return re;
}

int WhileStatement::analyze (TypeAnalyzer& ta) {
ta.pushScope();
int re = condition->analyze(ta) | loopStatement->analyze(ta);
ta.popScope();
return re;
}

int RepeatWhileStatement::analyze (TypeAnalyzer& ta) {
ta.pushScope();
int re = loopStatement->analyze(ta) | condition->analyze(ta);
ta.popScope();
return re;
}

int ReturnStatement::analyze (TypeAnalyzer& ta) {
int re = 0;
if (expr) re |= expr->analyze(ta);
auto method = ta.getCurMethod();
if (method && expr) method->returnType = ta.mergeTypes(method->returnType, expr->type);
return re;
}

int TryStatement::analyze (TypeAnalyzer& ta) {
ta.pushScope();
int re = tryPart->analyze(ta);
ta.popScope();
if (catchPart) {
ta.pushScope();
ta.findVariable(catchVar, LV_NEW);
re |= catchPart->analyze(ta);
ta.popScope();
}
if (finallyPart) {
ta.pushScope();
re |= finallyPart->analyze(ta);
ta.popScope();
}
return re;
}

int WithStatement::analyze (TypeAnalyzer& ta) {
QToken closeToken = { T_NAME, "close", 5, QV::UNDEFINED };
vector<shared_ptr<Variable>> varDecls = { make_shared<Variable>(varExpr, openExpr) };
auto varDecl = make_shared<VariableDeclaration>(varDecls);
auto closeExpr = BinaryOperation::create(varExpr, T_DOTQUEST, make_shared<NameExpression>(closeToken));
auto trySta = make_shared<TryStatement>(body, catchPart, closeExpr, catchVar);
vector<shared_ptr<Statement>> statements = { varDecl, trySta };
auto bs = make_shared<BlockStatement>(statements);
return bs->optimizeStatement()->analyze(ta);
}

int BlockStatement::analyze (TypeAnalyzer& ta) {
int re = 0;
if (makeScope) ta.pushScope();
for (auto sta: statements) {
re |= sta->analyze(ta);
}
if (makeScope) ta.popScope();
return re;
}

int ForStatement::analyze (TypeAnalyzer& ta) {
if (traditional) return analyzeTraditional(ta);
else return analyzeForEach(ta);
}

int ForStatement::analyzeForEach (TypeAnalyzer& ta) {
ta.pushScope();
int re = 0;
QToken iteratorToken = { T_NAME, "iterator", 8, QV::UNDEFINED };
QToken nextToken = { T_NAME, "next", 4, QV::UNDEFINED };
auto inVar = make_shared<NameExpression>(ta.createTempName(*inExpression));
ta.findVariable(inVar->token, LV_NEW);
BinaryOperation::create(inVar, T_EQ, BinaryOperation::create(inExpression, T_DOT, make_shared<NameExpression>(iteratorToken))) ->optimize() ->analyze(ta);
ta.pushScope();
shared_ptr<NameExpression> loopVariable = loopVariables.size()==1? dynamic_pointer_cast<NameExpression>(loopVariables[0]->name) : nullptr;
bool destructuring = !loopVariable;
if (destructuring) loopVariable = make_shared<NameExpression>(ta.createTempName(*loopVariables[0]->name));
ta.findVariable(loopVariable->token, LV_NEW);
auto nextCallExpr = BinaryOperation::create(inVar, T_DOT, make_shared<NameExpression>(nextToken));
BinaryOperation::create(loopVariable, T_EQ, nextCallExpr) ->optimize() ->analyze(ta);
if (destructuring) {
loopVariables[0]->value = loopVariable;
re |= make_shared<VariableDeclaration>(loopVariables)->optimizeStatement()->analyze(ta);
}
re |= loopStatement->analyze(ta);
ta.popScope();
ta.popScope();
return re;
}

int ForStatement::analyzeTraditional (TypeAnalyzer& ta) {
ta.pushScope();
int re = make_shared<VariableDeclaration>(loopVariables)->optimizeStatement()->analyze(ta);
ta.pushScope();
if (inExpression) re |= inExpression->analyze(ta);
re |= loopStatement->analyze(ta);
if (incrExpression) re |= incrExpression->analyze(ta);
ta.popScope();
ta.popScope();
return re;
}

int SwitchStatement::analyze (TypeAnalyzer& ta) {
ta.pushScope();
int re = make_shared<VariableDeclaration>(vector<shared_ptr<Variable>>({ make_shared<Variable>(var, expr) }))->optimizeStatement()->analyze(ta);
for (int i=0, n=cases.size(); i<n; i++) {
auto caseExpr = cases[i].first;
re |= caseExpr->analyze(ta);
ta.pushScope();
for (auto& s: cases[i].second) {
re |= s->analyze(ta);
}
ta.popScope();
}
ta.pushScope();
for (auto& s: defaultCase) {
re |= s->analyze(ta);
}
ta.popScope();
ta.popScope();
return re;
}

int VariableDeclaration::analyze (TypeAnalyzer& ta) {
int re = 0;
vector<shared_ptr<Variable>> destructured;
for (auto& var: vars) {
if (!var->name) continue;
auto name = dynamic_pointer_cast<NameExpression>(var->name);
if (!name) {
destructured.push_back(var);
vector<shared_ptr<NameExpression>> names;
for (auto& nm: decompose(ta, var->name, names)) {
ta.findVariable(nm->token, LV_NEW); 
}
continue;
}//if !name
AnalyzedVariable* lv = ta.findVariable(name->token, LV_NEW);
for (auto& decoration: decorations) re |= decoration->analyze(ta);
for (auto& decoration: var->decorations) re |= decoration->analyze(ta);
bool hoisted = false; //var->flags&VD_HOISTED;
if (var->value && !hoisted) {
if (!hoisted) re |= var->value->analyze(ta);
if (lv && var) {
lv->value = var->value;
if (var->value) lv->type = var->value->type;
}
}
}//for decompose
for (auto& var: destructured) {
auto assignable = dynamic_pointer_cast<Assignable>(var->name);
if (!assignable || !assignable->isAssignable()) continue;
re |= assignable->analyzeAssignment(ta, var->value);
}
return re;
}//end VariableDeclaration::analyze

int ImportDeclaration::analyze (TypeAnalyzer& ta) {
auto imp = imports;
QV imported = doCompileTimeImport(ta.parser.vm, ta.parser.filename, from);
if (importAll) {
if (imported.isNullOrUndefined()) return 0;
auto im = make_shared<LiteralMapExpression>(nearestToken());
for (auto [key, value]: imported.asObject<QMap>()->map) {
if (key.isString()) {
QString* qs = key.asObject<QString>();
QToken ctk = { T_NAME, qs->data, qs->length, key };
auto cst1 = make_shared<ConstantExpression>(ctk);
auto cst2 = make_shared<NameExpression>(ctk);
im->items.push_back(make_pair(cst1, cst2));
}}
int flags = VD_NODEFAULT;
auto vdim = make_shared<Variable>(im, nullptr, flags);
imp.push_back(vdim);
}
make_shared<VariableDeclaration>(imp)->optimizeStatement()->analyze(ta);
return false;
}


/*
void ExportDeclaration::analyze (TypeAnalyzer& ta) {
QToken exportsToken = { T_NAME, EXPORTS, 7, QV::UNDEFINED};
int subscriptSetterSymbol = ta.parser.vm.findMethodSymbol(("[]="));
bool multi = exports.size()>1;
int exportsSlot = ta.findLocalVariable(exportsToken, LV_EXISTING | LV_FOR_READ);
ta.writeOpLoadLocal(exportsSlot);
for (auto& p: exports) {
if (multi) ta.writeOp(OP_DUP);
ta.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, ta.findConstant(QV(QString::create(ta.parser.vm, p.first.start, p.first.length), QV_TAG_STRING)));
p.second->analyze(ta);
ta.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_3, subscriptSetterSymbol);
ta.writeOp(OP_POP);
}
if (multi) ta.writeOp(OP_POP);
}
*/



