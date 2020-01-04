#include "Parser.hpp"
#include "ParserRules.hpp"
#include "Statement.hpp"
#include "Expression.hpp"
#include "../vm/VM.hpp"
using namespace std;

void QParser::parseVarList (vector<shared_ptr<Variable>>& vars, int flags) {
do {
auto var = make_shared<Variable>(nullptr, nullptr, flags);
skipNewlines();
while (match(T_AT)) {
var->decorations.insert(var->decorations.begin(), parseExpression(P_PREFIX));
skipNewlines();
}
if (!(var->flags&VD_CONST) && match(T_CONST)) var->flags |= VD_CONST;
if (!(var->flags&VD_CONST)) match(T_VAR);
if (!(var->flags&VD_VARARG) && match(T_DOTDOTDOT)) var->flags |= VD_VARARG;
switch(nextToken().type){
case T_NAME: var->name = parseName(); break;
case T_LEFT_PAREN: var->name = parseGroupOrTuple(); var->value=make_shared<LiteralTupleExpression>(cur); break;
case T_LEFT_BRACKET: var->name = parseLiteralList(); var->value=make_shared<LiteralListExpression>(cur); break;
case T_LEFT_BRACE:  var->name = parseLiteralMap(); var->value=make_shared<LiteralMapExpression>(cur); break;
case T_UND: var->name = parseField(); break;
case T_UNDUND: var->name = parseStaticField(); break;
default: parseError("Expecting identifier, '(', '[' or '{' in variable declaration"); break;
}
if (!(var->flags&VD_VARARG) && match(T_DOTDOTDOT)) var->flags |= VD_VARARG;
skipNewlines();
if (!(flags&VD_NODEFAULT) && match(T_EQ)) var->value = parseExpression(P_COMPREHENSION);
else var->flags |= VD_NODEFAULT;
if (match(T_AS)) var->typeHint = parseTypeInfo();
vars.push_back(var);
if (flags&VD_SINGLE) break;
} while(match(T_COMMA));
}

shared_ptr<Statement> QParser::parseVarDecl () {
return parseVarDecl(cur.type==T_CONST? VD_CONST : 0);
}

shared_ptr<Statement> QParser::parseVarDecl (int flags) {
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VD_GLOBAL;
auto decl = make_shared<VariableDeclaration>();
parseVarList(decl->vars, flags);
return decl;
}

void QParser::parseFunctionParameters (shared_ptr<FunctionDeclaration>& func) {
if (func->flags&FD_METHOD) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
func->params.push_back( make_shared<Variable>(make_shared<NameExpression>(thisToken)));
}
if (match(T_LEFT_PAREN) && !match(T_RIGHT_PAREN)) {
parseVarList(func->params);
consume(T_RIGHT_PAREN, ("Expected ')' to close parameter list"));
}
else if (match(T_NAME)) {
prevToken();
parseVarList(func->params, VD_SINGLE);
}
if (match(T_AS)) func->returnTypeHint = parseTypeInfo();
if (func->params.size()>=1 && (func->params[func->params.size() -1]->flags&VD_VARARG)) func->flags |= FD_VARARG;
}

void QParser::parseDecoratedDecl (ClassDeclaration& cls, int flags) {
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
flags |= FD_STATIC;
nextToken();
}
const ParserRule& rule = rules[cur.type];
if (rule.member) (this->*rule.member)(cls, flags);
else { prevToken(); parseError("Expected declaration to decorate"); }
}
for (auto it=cls.methods.begin() + idxFrom, end = cls.methods.end(); it<end; ++it) (*it)->decorations = decorations;
}

void QParser::parseMethodDecl (ClassDeclaration& cls, int flags) {
prevToken();
QToken name = nextNameToken(true);
auto func = make_shared<FunctionDeclaration>(name, FD_METHOD | flags);
parseFunctionParameters(func);
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
if (auto m = cls.findMethod(name, flags&FD_STATIC)) {
parseError("%s already defined in line %d", string(name.start, name.length), getPositionOf(m->name.start).first);
}
match(T_COLON);
if (match(T_SEMICOLON)) func->body = make_shared<SimpleStatement>(cur);
else func->body = parseStatement();
if (!func->body) func->body = make_shared<SimpleStatement>(cur);
cls.methods.push_back(func);
}

void QParser::parseMethodDecl2 (ClassDeclaration& cls, int flags) {
nextToken();
parseMethodDecl(cls, flags);
}

void QParser::parseAsyncMethodDecl (ClassDeclaration& cls, int flags) {
if (nextToken().type==T_STATIC && !(flags&FD_STATIC)) {
flags |= FD_STATIC;
nextToken();
}
if (cur.type==T_FUNCTION) nextToken();
parseMethodDecl(cls, flags | FD_ASYNC);
}

void QParser::parseSimpleAccessor (ClassDeclaration& cls, int flags) {
if (cur.type==T_CONST) flags |= FD_CONST;
if (flags&FD_CONST) match(T_VAR);
do {
consume(T_NAME, ("Expected field name after 'var'"));
QToken fieldToken = cur;
string fieldName = string(fieldToken.start, fieldToken.length);
if (auto m = cls.findMethod(fieldToken, flags&FD_STATIC)) parseError("%s already defined in line %d", fieldName, getPositionOf(m->name.start).first);
cls.findField(flags&FD_STATIC? cls.staticFields : cls.fields, fieldToken);
shared_ptr<TypeInfo> typeHint = nullptr;
if (match(T_EQ)) {
auto& f = (flags&FD_STATIC? cls.staticFields : cls.fields)[fieldName];
f.defaultValue = parseExpression(P_COMPREHENSION);
}
if (match(T_AS)) typeHint = parseTypeInfo();
QString* setterName = QString::create(vm, fieldName+ ("="));
QToken setterNameToken = { T_NAME, setterName->data, setterName->length, QV(setterName, QV_TAG_STRING)  };
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED};
shared_ptr<NameExpression> thisExpr = make_shared<NameExpression>(thisToken);
shared_ptr<Expression> field;
flags |= FD_METHOD;
if (flags&FD_STATIC) field = make_shared<StaticFieldExpression>(fieldToken);
else field = make_shared<FieldExpression>(fieldToken);
shared_ptr<Expression> param = make_shared<NameExpression>(fieldToken);
auto thisParam = make_shared<Variable>(thisExpr);
auto setterParam = make_shared<Variable>(param);
setterParam->typeHint = typeHint;
vector<shared_ptr<Variable>> empty = { thisParam }, setterParams = { thisParam, setterParam  };
shared_ptr<Expression> assignment = BinaryOperation::create(field, T_EQ, param);
shared_ptr<FunctionDeclaration> getter = make_shared<FunctionDeclaration>(fieldToken, flags, empty, field);
shared_ptr<FunctionDeclaration> setter = make_shared<FunctionDeclaration>(setterNameToken, flags, setterParams, assignment);
getter->returnTypeHint = typeHint;
cls.methods.push_back(getter);
if (!(flags&FD_CONST)) cls.methods.push_back(setter);
} while (match(T_COMMA));
}

