#include "Parser.hpp"
#include "ParserRules.hpp"
#include "Statement.hpp"
#include "Expression.hpp"
#include "TypeInfo.hpp"
#include "../vm/VM.hpp"
using namespace std;

void QParser::parseVarList (vector<shared_ptr<Variable>>& vars, bitmask<VarFlag> flags) {
do {
auto var = make_shared<Variable>(nullptr, nullptr, flags);
skipNewlines();
while (match(T_AT)) {
var->decorations.insert(var->decorations.begin(), parseExpression(P_PREFIX));
skipNewlines();
}
if (!(var->flags &VarFlag::Const) && match(T_CONST)) var->flags |= VarFlag::Const;
if (!(var->flags & VarFlag::Const)) match(T_VAR);
if (!(var->flags & VarFlag::Vararg) && match(T_DOTDOTDOT)) var->flags |= VarFlag::Vararg;
switch(nextToken().type){
case T_NAME: var->name = parseName(); break;
case T_LEFT_PAREN: var->name = parseGroupOrTuple(); var->value=make_shared<LiteralTupleExpression>(cur); break;
case T_LEFT_BRACKET: var->name = parseLiteralList(); var->value=make_shared<LiteralListExpression>(cur); break;
case T_LEFT_BRACE:  var->name = parseLiteralMapOrSet(); var->value=make_shared<LiteralMapExpression>(cur); break;
case T_UND: var->name = parseField(); break;
case T_UNDUND: var->name = parseStaticField(); break;
default: parseError("Expecting identifier, '(', '[' or '{' in variable declaration"); break;
}
if (!(var->flags & VarFlag::Vararg) && match(T_DOTDOTDOT)) var->flags |= VarFlag::Vararg;
skipNewlines();
if (!(flags & VarFlag::NoDefault) && match(T_EQ)) var->value = parseExpression(P_COMPREHENSION);
else var->flags |= VarFlag::NoDefault;
if (match(T_AS)) var->type = parseTypeInfo();
vars.push_back(var);
if (flags & VarFlag::Single) break;
} while(match(T_COMMA));
}

shared_ptr<Statement> QParser::parseVarDecl () {
return parseVarDecl(cur.type==T_CONST? VarFlag::Const : VarFlag::None);
}

shared_ptr<Statement> QParser::parseVarDecl (bitmask<VarFlag> flags) {
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VarFlag::Global;
auto decl = make_shared<VariableDeclaration>();
parseVarList(decl->vars, flags);
return decl;
}

void QParser::parseFunctionParameters (shared_ptr<FunctionDeclaration>& func, ClassDeclaration* cld) {
if (func->flags &FuncDeclFlag::Method) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
auto var = make_shared<Variable>(make_shared<NameExpression>(thisToken));
if (cld) var->type = make_shared<ClassDeclTypeInfo>(cld);
func->params.push_back(var);
}
if (match(T_LEFT_PAREN) && !match(T_RIGHT_PAREN)) {
parseVarList(func->params, VarFlag::None);
consume(T_RIGHT_PAREN, ("Expected ')' to close parameter list"));
}
else if (match(T_NAME)) {
prevToken();
parseVarList(func->params, VarFlag::Single);
}
if (match(T_AS)) func->returnType = parseTypeInfo();
if (func->params.size()>=1 && (func->params[func->params.size() -1]->flags & VarFlag::Vararg)) func->flags |= FuncDeclFlag::Vararg;
}

void QParser::parseDecoratedDecl (ClassDeclaration& cls, bitmask<FuncDeclFlag> flags) {
vector<shared_ptr<Expression>> decorations;
int idxFrom = cls.methods.size();
bool parsed = false;
prevToken();
while (match(T_AT)) {
const char* c = in;
while(isSpace(*c) || isLine(*c)) c++;
if (*c=='(') {
parseMethodDecl(cls, flags);
parsed=true;
break;
}
skipNewlines();
auto decoration = parseExpression(P_PREFIX);
skipNewlines();
decorations.push_back(decoration);
}
if (!parsed) {
skipNewlines();
if (nextToken().type==T_STATIC) {
flags |= FuncDeclFlag::Static;
nextToken();
}
const ParserRule& rule = rules[cur.type];
if (rule.member) (this->*rule.member)(cls, flags);
else { prevToken(); parseError("Expected declaration to decorate"); }
}
for (auto it=cls.methods.begin() + idxFrom, end = cls.methods.end(); it<end; ++it) (*it)->decorations = decorations;
}

void QParser::parseMethodDecl (ClassDeclaration& cls, bitmask<FuncDeclFlag> flags) {
prevToken();
QToken name = nextNameToken(true);
auto func = make_shared<FunctionDeclaration>(vm, name, flags | FuncDeclFlag::Method );
parseFunctionParameters(func, &cls);
if (*name.start=='[' && func->params.size()<=1) {
parseError(("Subscript operator must take at least one argument"));
return;
}
if (*name.start=='[' && name.start[name.length -1]=='=' && func->params.size()<=2) {
parseError(("Subscript operator setter must take at least two arguments"));
return;
}
if (*name.start!='[' && name.start[name.length -1]=='=' && func->params.size()!=2) {
parseError(("Setter methods must take exactly one argument"));
}
if (auto m = cls.findMethod(name, static_cast<bool>(flags & FuncDeclFlag::Static))) {
parseError("%s already defined in line %d", string(name.start, name.length), getPositionOf(m->name.start).first);
}
match(T_COLON);
if (match(T_SEMICOLON)) func->body = make_shared<SimpleStatement>(cur);
else func->body = parseStatement();
if (!func->body) func->body = make_shared<SimpleStatement>(cur);
cls.methods.push_back(func);
}

void QParser::parseMethodDecl2 (ClassDeclaration& cls, bitmask<FuncDeclFlag> flags) {
nextToken();
parseMethodDecl(cls, flags);
}

void QParser::parseAsyncMethodDecl (ClassDeclaration& cls, bitmask<FuncDeclFlag> flags) {
if (nextToken().type==T_STATIC && !(flags &FuncDeclFlag::Static)) {
flags |= FuncDeclFlag::Static;
nextToken();
}
if (cur.type==T_FUNCTION) nextToken();
parseMethodDecl(cls, flags | FuncDeclFlag::Async);
}

void QParser::parseSimpleAccessor (ClassDeclaration& cls, bitmask<FuncDeclFlag> flags) {
flags |= FuncDeclFlag::Accessor;
if (cur.type==T_CONST) flags |= FuncDeclFlag::ReadOnly;
if (flags & FuncDeclFlag::ReadOnly) match(T_VAR);
do {
consume(T_NAME, ("Expected field name after 'var'"));
QToken fieldToken = cur;
string fieldName = string(fieldToken.start, fieldToken.length);
if (auto m = cls.findMethod(fieldToken, static_cast<bool>(flags &FuncDeclFlag::Static))) parseError("%s already defined in line %d", fieldName, getPositionOf(m->name.start).first);
int fieldIndex = cls.findField((flags & FuncDeclFlag::Static)? cls.staticFields : cls.fields, fieldToken);
shared_ptr<TypeInfo> typeHint = nullptr;
if (match(T_EQ)) {
auto& f = (flags & FuncDeclFlag::Static? cls.staticFields : cls.fields)[fieldName];
f.defaultValue = parseExpression(P_COMPREHENSION);
}
if (match(T_AS)) typeHint = parseTypeInfo();
QString* setterName = QString::create(vm, fieldName+ ("="));
QToken setterNameToken = { T_NAME, setterName->data, setterName->length, QV(setterName, QV_TAG_STRING)  };
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED};
shared_ptr<NameExpression> thisExpr = make_shared<NameExpression>(thisToken);
shared_ptr<Expression> field;
flags |= FuncDeclFlag::Method;
if (flags & FuncDeclFlag::Static) field = make_shared<StaticFieldExpression>(fieldToken);
else field = make_shared<FieldExpression>(fieldToken);
shared_ptr<Expression> param = make_shared<NameExpression>(fieldToken);
auto thisParam = make_shared<Variable>(thisExpr);
auto setterParam = make_shared<Variable>(param);
setterParam->type = typeHint;
vector<shared_ptr<Variable>> empty = { thisParam }, setterParams = { thisParam, setterParam  };
shared_ptr<Expression> assignment = BinaryOperation::create(field, T_EQ, param);
shared_ptr<FunctionDeclaration> getter = make_shared<FunctionDeclaration>(vm, fieldToken, flags, empty, field);
shared_ptr<FunctionDeclaration> setter = make_shared<FunctionDeclaration>(vm, setterNameToken, flags, setterParams, assignment);
getter->fieldIndex = fieldIndex;
getter->returnType = typeHint;
setter->fieldIndex = fieldIndex;
setter->returnType = typeHint;
cls.methods.push_back(getter);
if (!(flags & FuncDeclFlag::ReadOnly)) cls.methods.push_back(setter);
} while (match(T_COMMA));
}

