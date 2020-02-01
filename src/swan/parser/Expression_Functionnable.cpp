#include "Expression.hpp"
#include "Constants.hpp"
using namespace std;

static bool isItemFunctionnable (shared_ptr<Expression> item) {
shared_ptr<Expression> expr = item;
if (auto bop = dynamic_pointer_cast<BinaryOperation>(expr)) {
if (bop->op==T_EQ) expr = bop->left;
}
else if (auto th = dynamic_pointer_cast<TypeHintExpression>(expr)) expr = th->expr;
auto functionnable = dynamic_pointer_cast<Functionnable>(expr);
return functionnable && functionnable->isFunctionnable();
}

bool TypeHintExpression::isFunctionnable () {
return isItemFunctionnable(expr);
}

void TypeHintExpression::makeFunctionParameters (std::vector<std::shared_ptr<Variable>>& params) {
auto functionnable = dynamic_pointer_cast<Functionnable>(expr);
auto sizeBefore = params.size();
functionnable->makeFunctionParameters(params);
auto sizeAfter = params.size();
if (sizeAfter-sizeBefore==1) params.back()->typeHint = type;
}

bool LiteralTupleExpression::isFunctionnable () {
return all_of(items.begin(), items.end(), isItemFunctionnable);
}

void LiteralTupleExpression::makeFunctionParameters (vector<shared_ptr<Variable>>& params) {
for (auto& item: items) {
auto var = make_shared<Variable>(nullptr, nullptr);
auto bx = dynamic_pointer_cast<BinaryOperation>(item);
auto th = dynamic_pointer_cast<TypeHintExpression>(item);
if (bx && bx->op==T_EQ) {
var->name = bx->left;
var->value = bx->right;
th = dynamic_pointer_cast<TypeHintExpression>(bx->right);
}
else {
var->name = item;
var->flags |= VD_NODEFAULT;
}
if (th) var->typeHint = th->type;
params.push_back(var);
}}

bool LiteralListExpression::isFunctionnable () {
return all_of(items.begin(), items.end(), isItemFunctionnable);
}

void LiteralListExpression::makeFunctionParameters (vector<shared_ptr<Variable>>& params) {
auto var = make_shared<Variable>(shared_this(), make_shared<LiteralListExpression>(nearestToken()));
params.push_back(var);
}

bool LiteralMapExpression::isFunctionnable () {
return all_of(items.begin(), items.end(), [&](auto& item){ return isItemFunctionnable(item.second); });
}

void LiteralMapExpression::makeFunctionParameters (vector<shared_ptr<Variable>>& params) {
auto var = make_shared<Variable>(shared_this(), make_shared<LiteralMapExpression>(nearestToken()));
params.push_back(var);
}

void NameExpression::makeFunctionParameters (vector<shared_ptr<Variable>>& params) {
params.push_back(make_shared<Variable>(shared_this(), nullptr, VD_NODEFAULT));
}

bool LiteralSequenceExpression::isSingleSequence () {
if (items.size()!=1) return false;
auto expr = items[0];
if (expr->isComprehension()) return true;
auto binop = dynamic_pointer_cast<BinaryOperation>(expr);
return binop && (binop->op==T_DOTDOT || binop->op==T_DOTDOTDOT);
}
