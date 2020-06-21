#include "Parser.hpp"
#include "ParserRules.hpp"
#include "Statement.hpp"
#include "Expression.hpp"
#include "TypeInfo.hpp"
#include "../vm/VM.hpp"
using namespace std;

extern const QToken THIS_TOKEN;

void QParser::parseVarList (vector<shared_ptr<Variable>>& vars, bitmask<VarFlag> flags, bitmask<VarFlag> allowedFlags) {
do {
auto var = make_shared<Variable>(nullptr, nullptr, flags);
skipNewlines();
while (match(T_AT)) {
var->decorations.insert(var->decorations.begin(), parseExpression(P_PREFIX));
skipNewlines();
}
if (allowedFlags.value) {
parseKeywordFlags(var->flags, allowedFlags);
match(T_VAR);
}
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
if (!(var->flags & VarFlag::NoDefault) && match(T_EQ)) var->value = parseExpression(P_COMPREHENSION);
else var->flags |= VarFlag::NoDefault;
if (match(T_AS)) var->type = parseTypeInfo();
vars.push_back(var);
if (flags & VarFlag::Single) break;
} while(match(T_COMMA));
}

shared_ptr<Statement> QParser::parseVarDecl () {
return parseVarDecl(VarFlag::None);
}

shared_ptr<Statement> QParser::parseVarDecl (bitmask<VarFlag> flags) {
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VarFlag::Global;
auto decl = make_shared<VariableDeclaration>();
parseVarList(decl->vars, flags);
return decorateVarDecl(decl, flags);
}

void QParser::parseFunctionParameters (shared_ptr<FunctionDeclaration>& func, ClassDeclaration* cld) {
if (func->flags &VarFlag::Method) {
auto thisExpr = make_shared<NameExpression>(THIS_TOKEN);
auto thisVar = make_shared<Variable>(thisExpr);
if (cld) thisExpr->type = thisVar->type = make_shared<ClassDeclTypeInfo>(cld);
func->params.push_back(thisVar);
}
if (match(T_LEFT_PAREN) && !match(T_RIGHT_PAREN)) {
parseVarList(func->params, VarFlag::None, VarFlag::Const);
consume(T_RIGHT_PAREN, ("Expected ')' to close parameter list"));
}
else if (match(T_NAME)) {
prevToken();
parseVarList(func->params, VarFlag::Single);
}
if (match(T_AS)) func->returnType = parseTypeInfo();
if (func->params.size()>=1 && (func->params[func->params.size() -1]->flags & VarFlag::Vararg)) func->flags |= VarFlag::Vararg;
}

void QParser::parseDecoratedDecl (ClassDeclaration& cls, bitmask<VarFlag> flags) {
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
flags |= VarFlag::Static;
nextToken();
}
const ParserRule& rule = rules[cur.type];
if (rule.member) (this->*rule.member)(cls, flags);
else { prevToken(); parseError("Expected declaration to decorate"); }
}
for (auto it=cls.methods.begin() + idxFrom, end = cls.methods.end(); it<end; ++it) (*it)->decorations = decorations;
}

void QParser::parseMethodDecl (ClassDeclaration& cls, bitmask<VarFlag> flags) {
if (cur.type!=T_FUNCTION) prevToken();
QToken name = nextNameToken(true);
auto func = make_shared<FunctionDeclaration>(vm, name, flags | VarFlag::Method );
parseKeywordFlags(func->flags, VarFlag::Pure | VarFlag::Final | VarFlag::Static | VarFlag::Async);
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
if (auto m = cls.findMethod(name, static_cast<bool>(func->flags & VarFlag::Static))) {
parseError("%s already defined in line %d", name.str(), getPositionOf(m->name.start).first);
}
parseKeywordFlags(func->flags, VarFlag::Pure | VarFlag::Final | VarFlag::Static | VarFlag::Async);
match(T_COLON);
if (match(T_SEMICOLON)) func->body = make_shared<SimpleStatement>(cur);
else func->body = parseStatement();
if (!func->body) func->body = make_shared<SimpleStatement>(cur);
cls.methods.push_back(func);
}

void QParser::parseMemberDeclKeywords (ClassDeclaration& cls, bitmask<VarFlag> flags) {
parseKeywordFlags(flags, VarFlag::Static | VarFlag::Final | VarFlag::Async | VarFlag::ReadOnly);
skipNewlines();
const ParserRule& rule = rules[nextToken().type];
if (rule.member) (this->*rule.member)(cls, flags);
else parseError("Expected member name after modifiers");
}

void QParser::parseMemberDecl (ClassDeclaration& cls, bitmask<VarFlag> flags) {
if (flags & VarFlag::ReadOnly) parseSimpleAccessor(cls, flags);
else parseMethodDecl(cls, flags);
}

void QParser::parseSimpleAccessor (ClassDeclaration& cls, bitmask<VarFlag> flags) {
flags |= VarFlag::Accessor | VarFlag::Method;
if (cur.type==T_NAME) prevToken();
do {
consume(T_NAME, ("Expected field name after 'var'"));
QToken fieldToken = cur;
string fieldName = fieldToken.str();
if (auto m = cls.findMethod(fieldToken, static_cast<bool>(flags & VarFlag::Static))) parseError("%s already defined in line %d", fieldName, getPositionOf(m->name.start).first);
int fieldIndex = cls.findField((flags & VarFlag::Static)? cls.staticFields : cls.fields, fieldToken);
shared_ptr<TypeInfo> typeHint = nullptr;
if (match(T_EQ)) {
auto& f = (flags & VarFlag::Static? cls.staticFields : cls.fields)[fieldName];
f.defaultValue = parseExpression(P_COMPREHENSION);
}
if (match(T_AS)) typeHint = parseTypeInfo();
QString* setterName = QString::create(vm, fieldName+ ("="));
QToken setterNameToken = { T_NAME, setterName->data, setterName->length, QV(setterName, QV_TAG_STRING)  };
shared_ptr<NameExpression> thisExpr = make_shared<NameExpression>(THIS_TOKEN);
shared_ptr<Expression> field;
if (flags & VarFlag::Static) field = make_shared<StaticFieldExpression>(fieldToken);
else field = make_shared<FieldExpression>(fieldToken);
shared_ptr<Expression> param = make_shared<NameExpression>(fieldToken);
auto thisParam = make_shared<Variable>(thisExpr);
auto setterParam = make_shared<Variable>(param);
thisExpr->type = thisParam->type = make_shared<ClassDeclTypeInfo>(&cls, (flags & VarFlag::Static)? TypeInfoFlag::Static : TypeInfoFlag::None );
setterParam->type = typeHint;
vector<shared_ptr<Variable>> empty = { thisParam }, setterParams = { thisParam, setterParam  };
shared_ptr<Expression> assignment = BinaryOperation::create(field, T_EQ, param);
auto getter = make_shared<FunctionDeclaration>(vm, fieldToken, flags | VarFlag::Pure, empty, field);
auto setter = make_shared<FunctionDeclaration>(vm, setterNameToken, flags, setterParams, assignment);
getter->fieldIndex = fieldIndex;
getter->returnType = typeHint;
setter->fieldIndex = fieldIndex;
setter->returnType = typeHint;
cls.methods.push_back(getter);
if (!(flags & VarFlag::ReadOnly)) cls.methods.push_back(setter);
} while (match(T_COMMA));
}

shared_ptr<Statement> makeExportFromVarDecl (shared_ptr<VariableDeclaration> varDecl) {
auto exportDecl = make_shared<ExportDeclaration>();
auto name = varDecl->vars[0]->name;
exportDecl->exports.push_back(make_pair(name->nearestToken(), name));
vector<shared_ptr<Statement>> sta = { varDecl, exportDecl };
return make_shared<BlockStatement>(sta, false);
}

shared_ptr<Statement> QParser::decorateVarDecl (shared_ptr<VariableDeclaration> varDecl, bitmask<VarFlag> flags) {
if (flags & VarFlag::Export) return makeExportFromVarDecl(varDecl);
else return varDecl;
}

void QParser::parseKeywordFlags (bitmask<VarFlag>& flags, bitmask<VarFlag> allowedFlags) {
bool first = true;
do {
bitmask<VarFlag> flagToCheck;
switch(cur.type){
case T_CONST: flagToCheck = VarFlag::Const | VarFlag::Pure | VarFlag::ReadOnly; break;
case T_STATIC: flagToCheck = VarFlag::Static; break;
case T_FINAL: flagToCheck = VarFlag::Final; break;
case T_ASYNC: flagToCheck = VarFlag::Async; break;
case T_EXPORT: flagToCheck = VarFlag::Export; break;
case T_GLOBAL: flagToCheck = VarFlag::Global; break;
default: first=false; continue;
}
if (!(flagToCheck & allowedFlags)) {
parseError("%s not allowed at this place", cur.str());
}
else if (flags & flagToCheck) {
if (!first) parseError("Duplicated %s", cur.str());
}
flags |= (flagToCheck & allowedFlags);
first=false;
} while (matchOneOf(T_CONST, T_STATIC, T_FINAL, T_ASYNC, T_EXPORT, T_GLOBAL));
}

