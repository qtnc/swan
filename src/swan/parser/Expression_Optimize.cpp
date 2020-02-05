#include "Expression.hpp"
#include<cmath>
using namespace std;

unordered_map<int, QV(*)(double,double)> BASE_NUMBER_BINOPS = {
#define OP(T,O) { T_##T, [](double a, double b){ return QV(a O b); } }
#define OPB(T,O) { T_##T, [](double a, double b){ return QV(static_cast<double>(static_cast<int64_t>(a) O static_cast<int64_t>(b))); } }
#define OPF(T,F) { T_##T, [](double a, double b){ return QV(F(a,b)); } }
OP(PLUS, +), OP(MINUS, -),
OP(STAR, *), OP(SLASH, /),
OP(LT, <), OP(GT, >),
OP(LTE, <=), OP(GTE, >=),
OP(EQEQ, ==), OP(EXCLEQ, !=), OP(IS, ==),
OPB(BAR, |), OPB(AMP, &), OPB(CIRC, ^),
OPF(LTLT, dlshift), OPF(GTGT, drshift),
OPF(PERCENT, fmod), OPF(STARSTAR, pow), OPF(BACKSLASH, dintdiv)
#undef OP
#undef OPF
#undef OPB
};

unordered_map<int, double(*)(double)> BASE_NUMBER_UNOPS = {
{ T_MINUS, [](double x){ return -x; } },
{ T_PLUS, [](double x){ return +x; } },
{ T_TILDE, [](double x){ return static_cast<double>(~static_cast<int64_t>(x)); } }
#undef OP
};

shared_ptr<Expression> LiteralSequenceExpression::optimize () { 
for (auto& item: items) item = item->optimize(); 
return shared_this(); 
}

shared_ptr<Expression> LiteralMapExpression::optimize () { 
for (auto& p: items) { 
p.first = p.first->optimize(); 
p.second = p.second->optimize(); 
} 
return shared_this(); 
}

shared_ptr<Expression> ConditionalExpression::optimize () { 
condition=condition->optimize(); 
ifPart=ifPart->optimize(); 
elsePart=elsePart->optimize(); 
if (auto cst = dynamic_pointer_cast<ConstantExpression>(condition)) {
if (cst->token.value.isFalsy()) return elsePart;
else return ifPart;
}
return shared_this(); 
}

shared_ptr<Expression> SwitchExpression::optimize () {
expr=expr->optimize();
if (var) var = var->optimize();
if (defaultCase) defaultCase=defaultCase->optimize();
for (auto& c: cases) {
for (auto& i: c.first) i=i->optimize();
c.second=c.second->optimize();
}
return shared_this();
}

shared_ptr<Expression> UnpackExpression::optimize () { 
expr=expr->optimize(); 
return shared_this(); 
}

shared_ptr<Expression> AbstractCallExpression::optimize () { 
receiver=receiver->optimize(); 
for (auto& arg: args) arg=arg->optimize(); 
return shared_this(); 
}

shared_ptr<Expression> YieldExpression::optimize () { 
if (expr) expr=expr->optimize(); 
return shared_this(); 
}

shared_ptr<Expression> ImportExpression::optimize () { 
from=from->optimize(); 
return shared_this(); 
}

void Variable::optimize () {
name = name->optimize();
if (value) value = value->optimize();
for (auto& d: decorations) d = d->optimize();
}

shared_ptr<Expression> ClassDeclaration::optimize () { 
for (auto& m: methods) m=static_pointer_cast<FunctionDeclaration>(m->optimize()); 
return shared_this(); 
}

shared_ptr<Expression> FunctionDeclaration::optimize  () { 
body=body->optimizeStatement(); 
for (auto& param: params) param->optimize();
return shared_this(); 
}


shared_ptr<Expression> AssignmentOperation::optimize () {
if (optimized) return shared_this();
if (op>=T_PLUSEQ && op<=T_BARBAREQ) {
QTokenType newOp = static_cast<QTokenType>(op + T_PLUS - T_PLUSEQ);
right = BinaryOperation::create(left, newOp, right);
}
optimized=true;
return BinaryOperation::optimize();
}

shared_ptr<Expression> ComprehensionExpression::optimize () {
rootStatement = rootStatement->optimizeStatement();
return shared_this(); 
}

shared_ptr<Expression> UnaryOperation::optimize () { 
expr=expr->optimize();
if (auto cst = dynamic_pointer_cast<ConstantExpression>(expr)) {
QV& value = cst->token.value;
if (value.isNum() && BASE_NUMBER_UNOPS[op]) {
QToken token = cst->token;
token.value = BASE_NUMBER_UNOPS[op](value.d);
return make_shared<ConstantExpression>(token);
}
else if (op==T_EXCL) {
cst->token.value = value.isFalsy()? QV::TRUE : QV::FALSE;
return cst;
}
//other operations on non-number
}
return shared_this(); 
}

shared_ptr<Expression> BinaryOperation::optimize () { 
if (left && right && isComparison()) if (auto cc = optimizeChainedComparisons()) return cc->optimize();
if (left) left=left->optimize(); 
if (right) right=right->optimize();
if (!left || !right) return shared_this();
shared_ptr<ConstantExpression> c1 = dynamic_pointer_cast<ConstantExpression>(left), c2 = dynamic_pointer_cast<ConstantExpression>(right);
if (c1 && c2) {
QV &v1 = c1->token.value, &v2 = c2->token.value;
if (v1.isNum() && v2.isNum() && BASE_NUMBER_BINOPS[op]) {
QToken token = c1->token;
token.value = BASE_NUMBER_BINOPS[op](v1.d, v2.d);
return make_shared<ConstantExpression>(token);
}
else if (op==T_EQEQ) {
QToken token = c1->token;
token.value = v1.i==v2.i;
return make_shared<ConstantExpression>(token);
}
else if (op==T_EXCLEQ) {
QToken token = c1->token;
token.value = v1.i!=v2.i;
return make_shared<ConstantExpression>(token);
}
//other operations on non-number
}
else if (c1 && op==T_BARBAR) return c1->token.value.isFalsy()? right : left;
else if (c1 && op==T_QUESTQUEST) return c1->token.value.isNullOrUndefined()? right : left;
else if (c1 && op==T_AMPAMP) return c1->token.value.isFalsy()? left: right;
return shared_this(); 
}

shared_ptr<Expression> BinaryOperation::optimizeChainedComparisons () {
vector<shared_ptr<Expression>> all = { right };
vector<QTokenType> ops = { op };
shared_ptr<Expression> next = left;
while(true){
auto bop = dynamic_pointer_cast<BinaryOperation>(next);
if (bop && bop->left && bop->right && bop->isComparison()) { 
all.push_back(bop->right);
ops.push_back(bop->op);
next = bop->left;
}
else {
all.push_back(next);
break;
}
}
if (all.size()>=3) {
shared_ptr<Expression> expr = all.back();
all.pop_back(); 
expr = BinaryOperation::create(expr, ops.back(), all.back());
ops.pop_back();
while(all.size() && ops.size()) {
shared_ptr<Expression> l = all.back();
all.pop_back();
shared_ptr<Expression> r = all.back();
auto b = BinaryOperation::create(l, ops.back(), r);
ops.pop_back();
expr = BinaryOperation::create(expr, T_AMPAMP, b);
}
return expr->optimize();
}
return nullptr;
}
