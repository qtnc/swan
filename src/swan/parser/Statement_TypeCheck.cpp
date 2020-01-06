#include "Statement.hpp"
#include "Expression.hpp"
#include "TypeChecker.hpp"
#include "ParserRules.hpp"
#include "../vm/VM.hpp"
using namespace std;

/*
static vector<shared_ptr<NameExpression>>& decompose (TypeChecker& checker, shared_ptr<Expression> expr, vector<shared_ptr<NameExpression>>& names) {
if (auto name = dynamic_pointer_cast<NameExpression>(expr)) {
names.push_back(name);
return names;
}
if (auto seq = dynamic_pointer_cast<LiteralSequenceExpression>(expr)) {
for (auto& item: seq->items) decompose(checker, item, names);
return names;
}
if (auto map = dynamic_pointer_cast<LiteralMapExpression>(expr)) {
for (auto& item: map->items) decompose(checker, item.second, names);
return names;
}
auto bop = dynamic_pointer_cast<BinaryOperation>(expr);
if (bop && bop->op==T_EQ) {
decompose(checker, bop->left, names);
return names;
}
if (auto prop = dynamic_pointer_cast<GenericMethodSymbolExpression>(expr)) {
names.push_back(make_shared<NameExpression>(prop->token));
return names;
}
if (auto unpack = dynamic_pointer_cast<UnpackExpression>(expr)) {
decompose(checker, unpack->expr, names);
return names;
}
if (!dynamic_cast<FieldExpression*>(&*expr) && !dynamic_cast<StaticFieldExpression*>(&*expr)) checker.compileError(expr->nearestToken(), "Invalid target for assignment in destructuring");
return names;
}
*/

void IfStatement::typeCheck (TypeChecker& checker) {
condition->typeCheck(checker);
checker.pushScope();
ifPart->typeCheck(checker);
checker.popScope();
if (elsePart) {
checker.pushScope();
elsePart->typeCheck(checker);
checker.popScope();
}
}

void WhileStatement::typeCheck (TypeChecker& checker) {
checker.pushScope();
condition->typeCheck(checker);
loopStatement->typeCheck(checker);
checker.popScope();
}

void RepeatWhileStatement::typeCheck (TypeChecker& checker) {
checker.pushScope();
loopStatement->typeCheck(checker);
condition->typeCheck(checker);
checker.popScope();
}

void ReturnStatement::typeCheck (TypeChecker& checker) {
if (expr) expr->typeCheck(checker);
}

void ThrowStatement::typeCheck (TypeChecker& checker) {
if (expr) expr->typeCheck(checker);
}

void TryStatement::typeCheck (TypeChecker& checker) {
checker.pushScope();
tryPart->typeCheck(checker);
checker.popScope();
if (catchPart) {
checker.pushScope();
checker.findVariable(catchVar, LV_NEW);
catchPart->typeCheck(checker);
checker.popScope();
}
if (finallyPart) {
checker.pushScope();
finallyPart->typeCheck(checker);
checker.popScope();
}
}

void BlockStatement::typeCheck (TypeChecker& checker) {
if (makeScope) checker.pushScope();
for (auto sta: statements) sta->typeCheck(checker);
if (makeScope) checker.popScope();
}

/*
void ForStatement::typeCheck (TypeChecker& checker) {
if (traditional) compileTraditional(checker);
else compileForEach(checker);
}

void ForStatement::typeCheckForEach (TypeChecker& checker) {
checker.pushScope();
int iteratorSlot = checker.findVariable(checker.createTempName(), LV_NEW | LV_CONST);
int iteratorSymbol = checker.vm.findMethodSymbol(("iterator"));
int nextSymbol = checker.vm.findMethodSymbol(("next"));
int subscriptSymbol = checker.vm.findMethodSymbol(("[]"));
shared_ptr<NameExpression> loopVariable = loopVariables.size()==1? dynamic_pointer_cast<NameExpression>(loopVariables[0]->name) : nullptr;
bool destructuring = !loopVariable;
if (destructuring) loopVariable = make_shared<NameExpression>(checker.createTempName());
checker.writeDebugLine(inExpression->nearestToken());
inExpression->typeCheck(checker);
checker.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, iteratorSymbol);
checker.pushLoop();
checker.pushScope();
int valueSlot = checker.findVariable(loopVariable->token, LV_NEW);
int loopStart = checker.writePosition();
checker.loops.back().condPos = checker.writePosition();
checker.writeDebugLine(inExpression->nearestToken());
checker.writeOpLoadLocal(iteratorSlot);
checker.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, nextSymbol);
checker.loops.back().jumpsToPatch.push_back({ Loop::END, checker.writeOpJump(OP_JUMP_IF_UNDEFINED) });
if (destructuring) {
loopVariables[0]->value = loopVariable;
checker.writeDebugLine(inExpression->nearestToken());
make_shared<VariableDeclaration>(loopVariables)->optimizeStatement()->typeCheck(checker);
}
checker.writeDebugLine(loopStatement->nearestToken());
loopStatement->typeCheck(checker);
if (loopStatement->isExpression()) checker.writeOp(OP_POP);
checker.popScope();
checker.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
checker.loops.back().endPos = checker.writePosition();
checker.popLoop();
checker.popScope();
}

void ForStatement::typeCheckTraditional (TypeChecker& checker) {
checker.pushScope();
make_shared<VariableDeclaration>(loopVariables)->optimizeStatement()->typeCheck(checker);
checker.pushLoop();
checker.pushScope();
int loopStart = checker.writePosition();
inExpression->typeCheck(checker);
checker.loops.back().jumpsToPatch.push_back({ Loop::END, checker.writeOpJump(OP_JUMP_IF_FALSY) });
loopStatement->typeCheck(checker);
if (loopStatement->isExpression()) checker.writeOp(OP_POP);
checker.loops.back().condPos = checker.writePosition();
incrExpression->typeCheck(checker);
checker.writeOp(OP_POP);
checker.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
checker.loops.back().endPos = checker.writePosition();
checker.popScope();
checker.popLoop();
checker.popScope();
}
*/

void SwitchStatement::typeCheck (TypeChecker& checker) {
checker.pushScope();
make_shared<VariableDeclaration>(vector<shared_ptr<Variable>>({ make_shared<Variable>(var, expr) }))->optimizeStatement()->typeCheck(checker);
for (int i=0, n=cases.size(); i<n; i++) {
auto caseExpr = cases[i].first;
caseExpr->typeCheck(checker);
}
for (int i=0, n=cases.size(); i<n; i++) {
checker.pushScope();
for (auto& s: cases[i].second) s->typeCheck(checker);
checker.popScope();
}
checker.pushScope();
for (auto& s: defaultCase) s->typeCheck(checker);
checker.popScope();
checker.popScope();
}

/*
void VariableDeclaration::typeCheck (TypeChecker& checker) {
vector<shared_ptr<Variable>> destructured;
for (auto& var: vars) {
if (!var->name) continue;
auto name = dynamic_pointer_cast<NameExpression>(var->name);
if (!name) {
destructured.push_back(var);
vector<shared_ptr<NameExpression>> names;
for (auto& nm: decompose(checker, var->name, names)) {
if (var->flags&VD_GLOBAL) checker.findGlobalVariable(nm->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0));
else { checker.findVariable(nm->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0)); checker.writeOp(OP_LOAD_UNDEFINED); }
}
continue;
}
int slot = -1;
LocalVariable* lv = nullptr;
if ((var->flags&VD_GLOBAL)) slot = checker.findGlobalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0));
else slot = checker.findVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0), &lv);
for (auto& decoration: decorations) decoration->typeCheck(checker);
for (auto& decoration: var->decorations) decoration->typeCheck(checker);
if (var->value) {
if (auto fdecl = dynamic_pointer_cast<FunctionDeclaration>(var->value)) {
auto func = fdecl->compileFunction(checker);
func->name = string(name->token.start, name->token.length);
}
else var->value->typeCheck(checker);
if (lv) {
lv->value = var->value;
lv->type = var->value->getType(checker);
}
}
else checker.writeOp(OP_LOAD_UNDEFINED);
for (auto& decoration: var->decorations) checker.writeOp(OP_CALL_FUNCTION_1);
for (auto& decoration: decorations) checker.writeOp(OP_CALL_FUNCTION_1);
if (var->flags&VD_GLOBAL) {
checker.writeOpArg<uint_global_symbol_t>(OP_STORE_GLOBAL, slot);
checker.writeOp(OP_POP);
}
}//for decompose
for (auto& var: destructured) {
auto assignable = dynamic_pointer_cast<Assignable>(var->name);
if (!assignable || !assignable->isAssignable()) continue;
assignable->compileAssignment(checker, var->value);
checker.writeOp(OP_POP);
}
}//end VariableDeclaration::typeCheck
*/

void ImportDeclaration::typeCheck (TypeChecker& checker) {
doCompileTimeImport(checker.parser.vm, checker.parser.filename, from);
QToken importToken = { T_NAME, "import", 6, QV::UNDEFINED }, fnToken = { T_STRING, "", 0, QV(QString::create(checker.parser.vm, checker.parser.filename), QV_TAG_STRING) };
auto importName = make_shared<NameExpression>(importToken);
auto fnConst = make_shared<ConstantExpression>(fnToken);
vector<shared_ptr<Expression>> importArgs = { fnConst, from };
auto importCall = make_shared<CallExpression>(importName, importArgs);
imports[0]->value = importCall;
make_shared<VariableDeclaration>(imports)->optimizeStatement()->typeCheck(checker);
}

void ExportDeclaration::typeCheck (TypeChecker& checker) {
QToken exportsToken = { T_NAME, EXPORTS, 7, QV::UNDEFINED};
int subscriptSetterSymbol = checker.parser.vm.findMethodSymbol(("[]="));
bool multi = exports.size()>1;
auto exportsSlot = checker.findVariable(exportsToken, LV_EXISTING | LV_FOR_READ);
for (auto& p: exports) p.second->typeCheck(checker);
}



