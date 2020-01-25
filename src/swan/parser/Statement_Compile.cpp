#include "Statement.hpp"
#include "Expression.hpp"
#include "Compiler.hpp"
#include "ParserRules.hpp"
#include "../vm/VM.hpp"
using namespace std;

vector<shared_ptr<NameExpression>>& decompose (QCompiler& compiler, shared_ptr<Expression> expr, vector<shared_ptr<NameExpression>>& names) {
if (auto name = dynamic_pointer_cast<NameExpression>(expr)) {
names.push_back(name);
return names;
}
if (auto seq = dynamic_pointer_cast<LiteralSequenceExpression>(expr)) {
for (auto& item: seq->items) decompose(compiler, item, names);
return names;
}
if (auto map = dynamic_pointer_cast<LiteralMapExpression>(expr)) {
for (auto& item: map->items) decompose(compiler, item.second, names);
return names;
}
{
auto bop = dynamic_pointer_cast<BinaryOperation>(expr);
if (bop && bop->op==T_EQ) {
decompose(compiler, bop->left, names);
return names;
}}
if (auto th = dynamic_pointer_cast<TypeHintExpression>(expr)) {
decompose(compiler, th->expr, names);
return names;
}
if (auto prop = dynamic_pointer_cast<GenericMethodSymbolExpression>(expr)) {
names.push_back(make_shared<NameExpression>(prop->token));
return names;
}
if (auto unpack = dynamic_pointer_cast<UnpackExpression>(expr)) {
decompose(compiler, unpack->expr, names);
return names;
}
if (!dynamic_cast<FieldExpression*>(&*expr) && !dynamic_cast<StaticFieldExpression*>(&*expr)) compiler.compileError(expr->nearestToken(), "Invalid target for assignment in destructuring");
return names;
}

void IfStatement::compile (QCompiler& compiler) {
compiler.writeDebugLine(condition->nearestToken());
condition->compile(compiler);
int skipIfJump = compiler.writeOpJump(jumpType);
compiler.pushScope();
compiler.writeDebugLine(ifPart->nearestToken());
ifPart->compile(compiler);
if (ifPart->isExpression()) compiler.writeOp(OP_POP);
compiler.lastOp = OP_LOAD_UNDEFINED;
compiler.popScope();
if (elsePart) {
int skipElseJump = compiler.writeOpJump(OP_JUMP);
compiler.patchJump(skipIfJump);
compiler.pushScope();
compiler.writeDebugLine(elsePart->nearestToken());
elsePart->compile(compiler);
if (elsePart->isExpression()) compiler.writeOp(OP_POP);
compiler.lastOp = OP_LOAD_UNDEFINED;
compiler.popScope();
compiler.patchJump(skipElseJump);
}
else {
compiler.patchJump(skipIfJump);
}}

void WhileStatement::compile (QCompiler& compiler) {
compiler.writeDebugLine(condition->nearestToken());
compiler.pushLoop();
compiler.pushScope();
int loopStart = compiler.writePosition();
compiler.loops.back().condPos = compiler.writePosition();
condition->compile(compiler);
compiler.loops.back().jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP_IF_FALSY) });
compiler.writeDebugLine(loopStatement->nearestToken());
loopStatement->compile(compiler);
if (loopStatement->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
compiler.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
compiler.loops.back().endPos = compiler.writePosition();
compiler.popLoop();
}

void RepeatWhileStatement::compile (QCompiler& compiler) {
compiler.writeDebugLine(loopStatement->nearestToken());
compiler.pushLoop();
compiler.pushScope();
int loopStart = compiler.writePosition();
loopStatement->compile(compiler);
if (loopStatement->isExpression()) compiler.writeOp(OP_POP);
compiler.loops.back().condPos = compiler.writePosition();
compiler.writeDebugLine(condition->nearestToken());
condition->compile(compiler);
compiler.loops.back().jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP_IF_FALSY) });
compiler.popScope();
compiler.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
compiler.loops.back().endPos = compiler.writePosition();
compiler.popLoop();
}

void ContinueStatement::compile (QCompiler& compiler) {
if (compiler.loops.empty()) compiler.compileError(token, ("Can't use 'continue' outside of a loop"));
else if (count>compiler.loops.size()) compiler.compileError(token, ("Can't continue on that many loops"));
else {
compiler.writeDebugLine(nearestToken());
Loop& loop = *(compiler.loops.end() -count);
int varCount = compiler.countLocalVariablesInScope(loop.scope);
if (varCount>0) compiler.writeOpArg<uint_local_index_t>(OP_POP_SCOPE, varCount);
if (loop.condPos>=0) compiler.writeOpJumpBackTo(OP_JUMP_BACK, loop.condPos);
else loop.jumpsToPatch.push_back({ Loop::CONDITION, compiler.writeOpJump(OP_JUMP) });
}}

void BreakStatement::compile (QCompiler& compiler) {
if (compiler.loops.empty()) compiler.compileError(token, ("Can't use 'break' outside of a loop"));
else if (count>compiler.loops.size()) compiler.compileError(token, ("Can't break that many loops"));
else {
compiler.writeDebugLine(nearestToken());
Loop& loop = *(compiler.loops.end() -count);
int varCount = compiler.countLocalVariablesInScope(loop.scope);
if (varCount>0) compiler.writeOpArg<uint_local_index_t>(OP_POP_SCOPE, varCount);
loop.jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP) });
}}

void ReturnStatement::compile (QCompiler& compiler) {
compiler.writeDebugLine(nearestToken());
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
compiler.writeOp(OP_RETURN);
auto method = compiler.getCurMethod();
if (method && expr) method->returnTypeHint = compiler.mergeTypes(method->returnTypeHint, expr->getType(compiler));
}

void ThrowStatement::compile (QCompiler& compiler) {
compiler.writeDebugLine(nearestToken());
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
compiler.writeOp(OP_THROW);
}

void TryStatement::compile (QCompiler& compiler) {
compiler.pushScope();
int jumpPos=-1, tryPos = compiler.writeOpArg<uint64_t>(OP_TRY, 0xFFFFFFFFFFFFFFFFULL);
tryPart->compile(compiler);
if (tryPart->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
if (catchPart) {
jumpPos = compiler.writeOpJump(OP_JUMP);
uint32_t curPos = compiler.out.tellp();
compiler.out.seekp(tryPos);
compiler.out.write(reinterpret_cast<const char*>(&curPos), sizeof(uint32_t));
compiler.out.seekp(curPos);
compiler.pushScope();
compiler.findLocalVariable(catchVar, LV_NEW);
catchPart->compile(compiler);
if (catchPart->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
}
if (jumpPos>=0) compiler.patchJump(jumpPos);
uint32_t curPos = compiler.out.tellp();
compiler.out.seekp(tryPos+sizeof(uint32_t));
compiler.out.write(reinterpret_cast<const char*>(&curPos), sizeof(uint32_t));
compiler.out.seekp(curPos);
if (finallyPart) {
compiler.pushScope();
finallyPart->compile(compiler);
if (finallyPart->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
}
compiler.writeOp(OP_END_FINALLY);
}

void WithStatement::compile (QCompiler& compiler) {
QToken closeToken = { T_NAME, "close", 5, QV::UNDEFINED };
vector<shared_ptr<Variable>> varDecls = { make_shared<Variable>(varExpr, openExpr) };
auto varDecl = make_shared<VariableDeclaration>(varDecls);
auto closeExpr = BinaryOperation::create(varExpr, T_DOTQUEST, make_shared<NameExpression>(closeToken));
auto trySta = make_shared<TryStatement>(body, catchPart, closeExpr, catchVar);
vector<shared_ptr<Statement>> statements = { varDecl, trySta };
auto bs = make_shared<BlockStatement>(statements);
bs->optimizeStatement()->compile(compiler);
}

void BlockStatement::compile (QCompiler& compiler) {
if (makeScope) compiler.pushScope();
for (auto sta: statements) {
compiler.writeDebugLine(sta->nearestToken());
sta->compile(compiler);
if (sta->isExpression()) compiler.writeOp(OP_POP);
}
if (makeScope) compiler.popScope();
}

void ForStatement::compile (QCompiler& compiler) {
if (traditional) compileTraditional(compiler);
else compileForEach(compiler);
}

void ForStatement::compileForEach (QCompiler& compiler) {
compiler.pushScope();
int iteratorSlot = compiler.findLocalVariable(compiler.createTempName(), LV_NEW | LV_CONST);
int iteratorSymbol = compiler.vm.findMethodSymbol(("iterator"));
int nextSymbol = compiler.vm.findMethodSymbol(("next"));
int subscriptSymbol = compiler.vm.findMethodSymbol(("[]"));
shared_ptr<NameExpression> loopVariable = loopVariables.size()==1? dynamic_pointer_cast<NameExpression>(loopVariables[0]->name) : nullptr;
bool destructuring = !loopVariable;
if (destructuring) loopVariable = make_shared<NameExpression>(compiler.createTempName());
compiler.writeDebugLine(inExpression->nearestToken());
inExpression->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, iteratorSymbol);
compiler.pushLoop();
compiler.pushScope();
int valueSlot = compiler.findLocalVariable(loopVariable->token, LV_NEW);
int loopStart = compiler.writePosition();
compiler.loops.back().condPos = compiler.writePosition();
compiler.writeDebugLine(inExpression->nearestToken());
compiler.writeOpLoadLocal(iteratorSlot);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, nextSymbol);
compiler.loops.back().jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP_IF_UNDEFINED) });
if (destructuring) {
loopVariables[0]->value = loopVariable;
compiler.writeDebugLine(inExpression->nearestToken());
make_shared<VariableDeclaration>(loopVariables)->optimizeStatement()->compile(compiler);
}
compiler.writeDebugLine(loopStatement->nearestToken());
loopStatement->compile(compiler);
if (loopStatement->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
compiler.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
compiler.loops.back().endPos = compiler.writePosition();
compiler.popLoop();
compiler.popScope();
}

void ForStatement::compileTraditional (QCompiler& compiler) {
compiler.pushScope();
make_shared<VariableDeclaration>(loopVariables)->optimizeStatement()->compile(compiler);
compiler.pushLoop();
compiler.pushScope();
int loopStart = compiler.writePosition();
inExpression->compile(compiler);
compiler.loops.back().jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP_IF_FALSY) });
loopStatement->compile(compiler);
if (loopStatement->isExpression()) compiler.writeOp(OP_POP);
compiler.loops.back().condPos = compiler.writePosition();
incrExpression->compile(compiler);
compiler.writeOp(OP_POP);
compiler.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
compiler.loops.back().endPos = compiler.writePosition();
compiler.popScope();
compiler.popLoop();
compiler.popScope();
}

void SwitchStatement::compile (QCompiler& compiler) {
vector<int> jumps;
compiler.pushLoop();
compiler.pushScope();
compiler.writeDebugLine(expr->nearestToken());
make_shared<VariableDeclaration>(vector<shared_ptr<Variable>>({ make_shared<Variable>(var, expr) }))->optimizeStatement()->compile(compiler);
for (int i=0, n=cases.size(); i<n; i++) {
auto caseExpr = cases[i].first;
compiler.writeDebugLine(caseExpr->nearestToken());
caseExpr->compile(compiler);
jumps.push_back(compiler.writeOpJump(OP_JUMP_IF_TRUTY));
}
auto defaultJump = compiler.writeOpJump(OP_JUMP);
for (int i=0, n=cases.size(); i<n; i++) {
compiler.patchJump(jumps[i]);
compiler.pushScope();
for (auto& s: cases[i].second) {
s->compile(compiler);
if (s->isExpression()) compiler.writeOp(OP_POP);
}
compiler.popScope();
}
compiler.patchJump(defaultJump);
compiler.pushScope();
for (auto& s: defaultCase) {
s->compile(compiler);
if (s->isExpression()) compiler.writeOp(OP_POP);
}
compiler.popScope();
compiler.popScope();
compiler.loops.back().condPos = compiler.writePosition();
compiler.loops.back().endPos = compiler.writePosition();
compiler.popLoop();
}


void VariableDeclaration::compile (QCompiler& compiler) {
vector<shared_ptr<Variable>> destructured;
for (auto& var: vars) {
if (!var->name) continue;
auto name = dynamic_pointer_cast<NameExpression>(var->name);
if (!name) {
destructured.push_back(var);
vector<shared_ptr<NameExpression>> names;
for (auto& nm: decompose(compiler, var->name, names)) {
if (var->flags&VD_GLOBAL) compiler.findGlobalVariable(nm->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0));
else { compiler.findLocalVariable(nm->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0)); compiler.writeOp(OP_LOAD_UNDEFINED); }
}
continue;
}
int slot = -1;
LocalVariable* lv = nullptr;
if ((var->flags&VD_GLOBAL)) slot = compiler.findGlobalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0));
else slot = compiler.findLocalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0), &lv);
for (auto& decoration: decorations) decoration->compile(compiler);
for (auto& decoration: var->decorations) decoration->compile(compiler);
if (var->value) {
if (auto fdecl = dynamic_pointer_cast<FunctionDeclaration>(var->value)) {
auto func = fdecl->compileFunction(compiler);
func->name = string(name->token.start, name->token.length);
}
else var->value->compile(compiler);
if (lv) {
lv->value = var->value;
lv->type = var->value->getType(compiler);
}
}
else compiler.writeOp(OP_LOAD_UNDEFINED);
for (auto& decoration: var->decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
for (auto& decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
if (var->flags&VD_GLOBAL) {
compiler.writeOpArg<uint_global_symbol_t>(OP_STORE_GLOBAL, slot);
compiler.writeOp(OP_POP);
}
}//for decompose
for (auto& var: destructured) {
auto assignable = dynamic_pointer_cast<Assignable>(var->name);
if (!assignable || !assignable->isAssignable()) continue;
assignable->compileAssignment(compiler, var->value);
compiler.writeOp(OP_POP);
}
}//end VariableDeclaration::compile

void ImportDeclaration::compile (QCompiler& compiler) {
doCompileTimeImport(compiler.parser.vm, compiler.parser.filename, from);
QToken importToken = { T_NAME, "import", 6, QV::UNDEFINED }, fnToken = { T_STRING, "", 0, QV(QString::create(compiler.parser.vm, compiler.parser.filename), QV_TAG_STRING) };
auto importName = make_shared<NameExpression>(importToken);
auto fnConst = make_shared<ConstantExpression>(fnToken);
vector<shared_ptr<Expression>> importArgs = { fnConst, from };
auto importCall = make_shared<CallExpression>(importName, importArgs);
imports[0]->value = importCall;
make_shared<VariableDeclaration>(imports)->optimizeStatement()->compile(compiler);
}


void ExportDeclaration::compile (QCompiler& compiler) {
QToken exportsToken = { T_NAME, EXPORTS, 7, QV::UNDEFINED};
int subscriptSetterSymbol = compiler.parser.vm.findMethodSymbol(("[]="));
bool multi = exports.size()>1;
int exportsSlot = compiler.findLocalVariable(exportsToken, LV_EXISTING | LV_FOR_READ);
compiler.writeOpLoadLocal(exportsSlot);
for (auto& p: exports) {
if (multi) compiler.writeOp(OP_DUP);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, p.first.start, p.first.length), QV_TAG_STRING)));
p.second->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_3, subscriptSetterSymbol);
compiler.writeOp(OP_POP);
}
if (multi) compiler.writeOp(OP_POP);
}



