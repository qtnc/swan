#include "Statement.hpp"
#include "Expression.hpp"
#include "Constants.hpp"
using namespace std;

void IfStatement::chain (const shared_ptr<Statement>& st) { 
if (!ifPart) ifPart=st; 
else if (!elsePart) elsePart = st;
}

shared_ptr<Statement> IfStatement::optimizeStatement () { 
condition=condition->optimize(); 
ifPart=ifPart->optimizeStatement(); 
if (elsePart) elsePart=elsePart->optimizeStatement(); 
if (auto u = dynamic_pointer_cast<UnaryOperation>(condition)) if (u->op==T_EXCL) { jumpType=OP_JUMP_IF_TRUTY; condition = u->expr; }
if (auto c = dynamic_pointer_cast<ConstantExpression>(condition)) {
auto singlePart = c->token.value.isFalsy()? elsePart : ifPart;
if (!singlePart) singlePart = make_shared<SimpleStatement>(c->token);
return singlePart;
}
return shared_this(); 
}

shared_ptr<Statement> SwitchStatement::optimizeStatement () { 
expr = expr->optimize();
for (auto& p: cases) { 
p.first = p.first->optimize(); 
for (auto& s: p.second) s = s->optimizeStatement();
}
for (auto& s: defaultCase) s=s->optimizeStatement();
return shared_this(); 
}

void ForStatement::chain (const shared_ptr<Statement>& st) { 
loopStatement=st; 
}

shared_ptr<Statement> ForStatement::optimizeStatement () { 
if (inExpression) inExpression=inExpression->optimize(); 
if (loopStatement) loopStatement=loopStatement->optimizeStatement();
if (incrExpression) incrExpression=incrExpression->optimize();
for (auto& lv: loopVariables) lv->optimize();
return shared_this(); 
}

shared_ptr<Statement> WhileStatement::optimizeStatement () { 
condition=condition->optimize(); 
loopStatement=loopStatement->optimizeStatement(); 
if (auto cst = dynamic_pointer_cast<ConstantExpression>(condition)) { if (cst->token.value.isFalsy()) return make_shared<SimpleStatement>(nearestToken()); }
return shared_this(); 
}

shared_ptr<Statement> RepeatWhileStatement::optimizeStatement () { 
condition=condition->optimize(); 
loopStatement=loopStatement->optimizeStatement(); 
return shared_this(); 
}

shared_ptr<Statement> ReturnStatement::optimizeStatement () { 
if (expr) expr=expr->optimize(); 
return shared_this(); 
}

shared_ptr<Statement> ThrowStatement::optimizeStatement () { 
if (expr) expr=expr->optimize(); 
return shared_this(); 
}

void TryStatement::chain (const shared_ptr<Statement>& st) { 
tryPart=st; 
}

shared_ptr<Statement> TryStatement::optimizeStatement () { 
tryPart = tryPart->optimizeStatement();
if (catchPart) catchPart=catchPart->optimizeStatement();
if (finallyPart) finallyPart = finallyPart->optimizeStatement();
return shared_this(); 
}

void BlockStatement::chain (const shared_ptr<Statement>& st) { 
statements.push_back(st); 
}

shared_ptr<Statement> VariableDeclaration::optimizeStatement () { 
for (auto& v: vars) v->optimize();
return shared_this(); 
}

shared_ptr<Statement> ExportDeclaration::optimizeStatement () { 
for (auto& v: exports) v.second=v.second->optimize(); 
return shared_this(); 
}

shared_ptr<Statement> ImportDeclaration::optimizeStatement () { 
from=from->optimize(); 
return shared_this(); 
}

shared_ptr<Statement> BlockStatement::optimizeStatement () { 
if (optimized) return shared_this(); 
for (auto& sta: statements) sta=sta->optimizeStatement(); 
doHoisting();
optimized = true;
return shared_this(); 
}

void BlockStatement::doHoisting () {
vector<shared_ptr<Statement>> statementsToAdd;
for (auto& sta: statements) {
auto vd = dynamic_pointer_cast<VariableDeclaration>(sta);
if (vd  && vd->vars.size()==1
&& !(vd->vars[0]->flags&VD_CONST)
&& dynamic_pointer_cast<NameExpression>(vd->vars[0]->name) 
&& (dynamic_pointer_cast<FunctionDeclaration>(vd->vars[0]->value) || dynamic_pointer_cast<ClassDeclaration>(vd->vars[0]->value))
) {
auto name = vd->vars[0]->name;
auto value = vd->vars[0]->value;
vd->vars[0]->value = nullptr;
statementsToAdd.push_back(vd->optimizeStatement());
sta = BinaryOperation::create(name, T_EQ, value) ->optimize();
}}
if (statementsToAdd.size()) statements.insert(statements.begin(), statementsToAdd.begin(), statementsToAdd.end());
}


bool BlockStatement::isUsingExports () { 
return any_of(statements.begin(), statements.end(), [&](auto s){ return s && s->isUsingExports(); }); 
}

