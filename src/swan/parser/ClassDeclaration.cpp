#include "Constants.hpp"
#include "Expression.hpp"
#include "Statement.hpp"
#include "TypeInfo.hpp"
#include "FunctionInfo.hpp"
#include "ParserRules.hpp"
#include "Compiler.hpp"
#include "TypeAnalyzer.hpp"
#include "../vm/VM.hpp"
using namespace std;

int ClassDeclaration::findField (unordered_map<string,Field>& flds, const QToken& name, shared_ptr<TypeInfo>** type) {
auto it = flds.find(string(name.start, name.length));
int index = -1;
if (it!=flds.end()) {
index = it->second.index;
if (type) *type = &(it->second.type);
}
else {
index = flds.size();
flds[string(name.start, name.length)] = { index, name, nullptr, nullptr };
if (type) *type = &(flds[string(name.start, name.length)].type);
}
return index;
}

void ClassDeclaration::handleAutoConstructor (QCompiler& compiler, unordered_map<string,Field>& memberFields, bool isStatic) {
if (all_of(methods.begin(), methods.end(), [&](auto& m){ return isStatic!=!!(m->flags &FuncDeclFlag::Static); })) return;
auto inits = make_shared<BlockStatement>();
vector<pair<string,Field>> initFields;
for (auto& field: memberFields) if (field.second.defaultValue) initFields.emplace_back(field.first, field.second);
sort(initFields.begin(), initFields.end(), [&](auto& a, auto& b){ return a.second.index<b.second.index; });
for (auto& fp: initFields) {
auto& f = fp.second;
shared_ptr<Expression> fieldExpr;
if (isStatic) fieldExpr = make_shared<StaticFieldExpression>(f.token);
else fieldExpr = make_shared<FieldExpression>(f.token);
auto assignment = BinaryOperation::create(fieldExpr, T_QUESTQUESTEQ, f.defaultValue) ->optimize();
inits->statements.push_back(assignment);
}
QToken ctorToken = { T_NAME, CONSTRUCTOR, 11, QV::UNDEFINED };
auto ctor = findMethod(ctorToken, isStatic);
if (!ctor && (!isStatic || inits->statements.size() )) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
auto thisExpr = make_shared<NameExpression>(thisToken);
ctor = make_shared<FunctionDeclaration>(compiler.vm, ctorToken);
ctor->flags.set(FuncDeclFlag::Static, isStatic);
ctor->params.push_back(make_shared<Variable>(thisExpr));
if (isStatic) ctor->body = make_shared<SimpleStatement>(ctorToken);
else {
auto arg = make_shared<NameExpression>(compiler.createTempName(*this)); 
ctor->params.push_back(make_shared<Variable>(arg, nullptr, VarFlag::Vararg )); 
ctor->flags |= FuncDeclFlag::Vararg;
ctor->body = BinaryOperation::create(make_shared<SuperExpression>(ctorToken), T_DOT, make_shared<CallExpression>(make_shared<NameExpression>(ctorToken), vector<shared_ptr<Expression>>({ make_shared<UnpackExpression>(arg) }) ));
}
methods.push_back(ctor);
}
if (ctor && inits->statements.size()) {
inits->chain(ctor->body);
ctor->body = inits;
}}


