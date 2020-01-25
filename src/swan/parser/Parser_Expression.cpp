#include "Parser.hpp"
#include "ParserRules.hpp"
#include "Expression.hpp"
#include "Statement.hpp"
#include "../vm/VM.hpp"
using namespace std;

shared_ptr<Expression> QParser::nameExprToConstant (shared_ptr<Expression> key) {
if (auto bop = dynamic_pointer_cast<BinaryOperation>(key)) {
if (bop->op==T_EQ) key = bop->left;
while (bop && bop->op==T_DOT) {
key = bop->right;
bop = dynamic_pointer_cast<BinaryOperation>(key);
}}
if (auto th = dynamic_pointer_cast<TypeHintExpression>(key)) key = th->expr;
if (auto name = dynamic_pointer_cast<NameExpression>(key)) {
QToken token = name->token;
token.type = T_STRING;
token.value = QV(vm, string(token.start, token.length));
key = make_shared<ConstantExpression>(token);
}
return key;
}

shared_ptr<BinaryOperation> BinaryOperation::create (shared_ptr<Expression> left, QTokenType op, shared_ptr<Expression> right) {
if (rules[op].flags&P_SWAP_OPERANDS) swap(left, right);
switch(op){
case T_EQ:
case T_PLUSEQ: case T_MINUSEQ: case T_STAREQ: case T_STARSTAREQ: case T_SLASHEQ: case T_BACKSLASHEQ: case T_PERCENTEQ: case T_ATEQ:
case T_BAREQ: case T_AMPEQ: case T_CIRCEQ: case T_AMPAMPEQ: case T_BARBAREQ: case T_LTLTEQ: case T_GTGTEQ: case T_QUESTQUESTEQ:
return make_shared<AssignmentOperation>(left, op, right);
case T_DOT: 
return make_shared<MemberLookupOperation>(left, right);
case T_COLONCOLON: 
return make_shared<MethodLookupOperation>(left, right);
case T_AMPAMP: case T_QUESTQUEST: case T_BARBAR:
return make_shared<ShortCircuitingBinaryOperation>(left, op, right);
case T_DOTQUEST:
return BinaryOperation::create(left, T_AMPAMP, BinaryOperation::create(left, T_DOT, right));
default: 
return make_shared<BinaryOperation>(left, op, right);
}}

shared_ptr<Expression> QParser::parseYield () {
shared_ptr<Expression> expr = nullptr;
QToken tk = cur;
skipNewlines();
if (matchOneOf(T_RIGHT_BRACE, T_LINE, T_SEMICOLON)) prevToken();
else expr = parseExpression();
return make_shared<YieldExpression>(tk, expr);
}



shared_ptr<Expression> QParser::parseDecoratedExpression () {
auto tp = nextToken().type;
if (tp==T_NUM) return make_shared<AnonymousLocalExpression>(cur);
else if (((tp>=T_PLUS && tp<=T_GTE) || (tp>=T_DOT && tp<=T_DOTQUEST)) && rules[tp].infix) {
prevToken();
cur.type=T_NAME;
prevToken();
auto func = make_shared<FunctionDeclaration>(vm, cur);
auto nm = make_shared<NameExpression>(cur);
func->params.push_back(make_shared<Variable>(nm));
func->body = parseExpression(P_COMPREHENSION);
return func;
}
else prevToken();
auto decoration = parseExpression(P_PREFIX);
auto expr = parseExpression();
if (expr->isDecorable()) {
auto decorable = dynamic_pointer_cast<Decorable>(expr);
decorable->decorations.insert(decorable->decorations.begin(), decoration);
}
else parseError("Expression can't be decorated");
return expr;
}

shared_ptr<Expression> QParser::parseLambda  () {
return parseLambda(0);
}

shared_ptr<Expression> QParser::parseLambda  (int flags) {
auto func = make_shared<FunctionDeclaration>(vm, cur);
func->flags |= flags;
if (match(T_GT)) func->flags |= FD_METHOD;
if (match(T_STAR)) func->flags |= FD_FIBER;
else if (match(T_AMP)) flags |= FD_ASYNC;
parseFunctionParameters(func);
match(T_COLON);
func->body = parseStatement();
if (!func->body) func->body = make_shared<SimpleStatement>(cur);
return func;
}

shared_ptr<Expression> QParser::parseArrowFunction (shared_ptr<Expression> fargs) {
auto functionnable = dynamic_pointer_cast<Functionnable>(fargs);
if (!functionnable || !functionnable->isFunctionnable()) {
parseError("Expression can't be considered as the argument list for an anonymous function");
return fargs;
}
auto func = make_shared<FunctionDeclaration>(vm, cur);
if (match(T_GT)) func->flags |= FD_METHOD;
if (match(T_STAR)) func->flags |= FD_FIBER;
else if (match(T_AMP)) func->flags |= FD_ASYNC;
if (func->flags&FD_METHOD) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
func->params.push_back( make_shared<Variable>(make_shared<NameExpression>(thisToken)));
}
functionnable->makeFunctionParameters(func->params);
func->body = parseStatement();
return func;
}


shared_ptr<Expression> QParser::parseImportExpression () {
bool parent = match(T_LEFT_PAREN);
auto result = make_shared<ImportExpression>(parseExpression());
if (parent) consume(T_RIGHT_PAREN, "Expected ')' to close function call");
return result;
}

shared_ptr<Expression> QParser::parsePrefixOp () {
QTokenType op =  cur.type;
shared_ptr<Expression> right = parseExpression(P_PREFIX);
return make_shared<UnaryOperation>(op, right);
}

shared_ptr<Expression> QParser::parseInfixOp (shared_ptr<Expression> left) {
QTokenType op =  cur.type;
auto& rule = rules[op];
auto priority = rule.priority;
if (rule.flags&P_RIGHT) --priority;
shared_ptr<Expression> right = parseExpression(priority);
return BinaryOperation::create(left, op, right);
}

shared_ptr<Expression> QParser::parseInfixIs (shared_ptr<Expression> left) {
bool negate = match(T_EXCL);
auto& rule = rules[T_IS];
auto priority = rule.priority;
if (rule.flags&P_RIGHT) --priority;
shared_ptr<Expression> right = parseExpression(priority);
right = BinaryOperation::create(left, T_IS, right);
if (negate) right = make_shared<UnaryOperation>(T_EXCL, right);
return right;
}

shared_ptr<Expression> QParser::parseInfixNot (shared_ptr<Expression> left) {
consume(T_IN, "Expected 'in' after infix not");
auto& rule = rules[T_IN];
auto priority = rule.priority;
if (rule.flags&P_RIGHT) --priority;
shared_ptr<Expression> right = parseExpression(priority);
return make_shared<UnaryOperation>(T_EXCL, BinaryOperation::create(left, T_IN, right));
}

shared_ptr<Expression> QParser::parseTypeHint (shared_ptr<Expression> expr) {
auto th = parseTypeInfo();
return make_shared<TypeHintExpression>(expr, th);
}

shared_ptr<Expression> QParser::parseConditional  (shared_ptr<Expression> cond) {
shared_ptr<Expression> ifPart = parseExpression();
skipNewlines();
consume(T_COLON, ("Expected ':' between conditional branches"));
shared_ptr<Expression> elsePart = parseExpression();
return make_shared<ConditionalExpression>(cond, ifPart, elsePart);
}

shared_ptr<Expression> QParser::parseComprehension (shared_ptr<Expression> loopExpr) {
skipNewlines();
shared_ptr<ForStatement> firstFor = make_shared<ForStatement>(cur);
firstFor->parseHead(*this);
shared_ptr<Comprenable> expr = firstFor;
shared_ptr<Statement> rootStatement = firstFor, leafExpr = make_shared<YieldExpression>(loopExpr->nearestToken(), loopExpr);
while(true){
skipNewlines();
if (match(T_FOR)) {
shared_ptr<ForStatement> forSta = make_shared<ForStatement>(cur);
forSta->parseHead(*this);
expr->chain(forSta);
expr = forSta;
}
else if (match(T_IF)) {
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
expr->chain(ifSta);
expr = ifSta;
}
else if (match(T_WHILE)) {
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
ifSta->elsePart = make_shared<ReturnStatement>(cur);
expr->chain(ifSta);
expr = ifSta;
}
else if (match(T_BREAK)) {
consume(T_IF, "Expected 'if' after 'break' in comprehension expression");
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
ifSta->chain(make_shared<BreakStatement>(cur));
expr->chain(ifSta);
expr = ifSta;
}
else if (match(T_RETURN)) {
consume(T_IF, "Expected 'if' after 'return' in comprehension expression");
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
ifSta->chain(make_shared<ReturnStatement>(cur));
expr->chain(ifSta);
expr = ifSta;
}
else if (match(T_CONTINUE)) {
if (match(T_IF)) {
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
ifSta->chain(make_shared<ContinueStatement>(cur));
expr->chain(ifSta);
expr = ifSta;
} else {
consume(T_WHILE, "Expected 'if' or 'while' after 'continue' in comprehension expression");
auto cond = parseExpression(P_COMPREHENSION);
QToken trueToken = { T_TRUE, nullptr, 0, true }, falseToken = { T_FALSE, nullptr, 0, false };
auto var = make_shared<NameExpression>(createTempName());
vector<shared_ptr<Variable>> vars = { make_shared<Variable>(var, make_shared<ConstantExpression>(trueToken) ) };
vector<shared_ptr<Statement>> rootBlock = { make_shared<VariableDeclaration>(vars), rootStatement }, leafBlock = {
make_shared<IfStatement>(var, 
make_shared<IfStatement>(cond, make_shared<ContinueStatement>(cur), BinaryOperation::create(var, T_EQ, make_shared<ConstantExpression>(falseToken) ) 
))};
auto bs = make_shared<BlockStatement>(leafBlock);
rootStatement = make_shared<BlockStatement>(rootBlock);
expr->chain(bs);
expr = bs;
}}
else if (match(T_WITH)) {
auto withSta = make_shared<WithStatement>();
withSta->parseHead(*this);
withSta->parseTail(*this);
expr->chain(withSta);
expr = withSta;
}
else break;
}
expr->chain(leafExpr);
return make_shared<ComprehensionExpression>(rootStatement, loopExpr);
}

shared_ptr<Expression> QParser::parseMethodCall (shared_ptr<Expression> receiver) {
vector<shared_ptr<Expression>> args;
shared_ptr<LiteralMapExpression> mapArg = nullptr;
if (!match(T_RIGHT_PAREN)) {
do {
shared_ptr<Expression> arg = parseUnpackOrExpression();
if (match(T_COLON)) {
shared_ptr<Expression> val;
if (matchOneOf(T_COMMA, T_RIGHT_PAREN, T_RIGHT_BRACKET, T_RIGHT_BRACE)) { val=arg; prevToken(); }
else val = parseExpression();
if (!mapArg) { mapArg = make_shared<LiteralMapExpression>(cur); args.push_back(mapArg); }
arg = nameExprToConstant(arg);
mapArg->items.push_back(make_pair(arg, val));
}
else if (arg) args.push_back(arg);
} while(match(T_COMMA));
skipNewlines();
consume(T_RIGHT_PAREN, ("Expected ')' to close method call"));
}
if (args.size() >= std::numeric_limits<uint_local_index_t>::max() -2) parseError("Too many arguments passed");
return make_shared<CallExpression>(receiver, args);
}

shared_ptr<Expression> QParser::parseSubscript  (shared_ptr<Expression> receiver) {
vector<shared_ptr<Expression>> args;
do {
shared_ptr<Expression> arg = parseExpression();
if (arg) args.push_back(arg);
} while(match(T_COMMA));
skipNewlines();
consume(T_RIGHT_BRACKET, ("Expected ']' to close subscript"));
if (args.size() >= std::numeric_limits<uint_local_index_t>::max() -2) parseError("Too many arguments passed");
return make_shared<SubscriptExpression>(receiver, args);
}

shared_ptr<Expression> QParser::parseSwitchCase (shared_ptr<Expression> left) {
auto& rule = rules[nextToken().type];
if (rule.infix && rule.priority>=P_COMPARISON) {
return (this->*rule.infix)(left);
}
else if (rule.prefix && rule.priority>=P_COMPARISON) {
shared_ptr<Expression> right = (this->*rule.prefix)();
return BinaryOperation::create(left, T_EQEQ, right);
}
else parseError("Expected literal, identifier or infix expression after 'case'");
return nullptr;
}

shared_ptr<Expression> QParser::parseSwitchExpression () {
auto sw = make_shared<SwitchExpression>();
pair<vector<shared_ptr<Expression>>, shared_ptr<Expression>>* activeCase = nullptr;
shared_ptr<Expression>* activeExpr = nullptr;
bool defaultDefined = false;
sw->var = make_shared<DupExpression>(cur);
sw->expr = parseExpression(P_COMPREHENSION);
skipNewlines();
consume(T_LEFT_BRACE, "Expected '{' to begin switch");
while(true){
skipNewlines();
if (match(T_CASE)) {
if (defaultDefined) parseError("Default case must appear last");
sw->cases.emplace_back();
activeCase = &sw->cases.back();
activeExpr = &activeCase->second;
activeCase->first.push_back(parseSwitchCase(sw->var));
while(match(T_COMMA)) activeCase->first.push_back(parseSwitchCase(sw->var));
match(T_COLON);
}
else if (match(T_DEFAULT)) {
if (defaultDefined) parseError("Duplicated default case");
defaultDefined=true;
activeCase = nullptr;
activeExpr = &sw->defaultCase;
match(T_COLON);
}
else if (match(T_RIGHT_BRACE)) break;
else if (match(T_END)) { result=CR_INCOMPLETE; break; }
else {
if (!activeCase) { parseError("Expected 'case' after beginnig of switch expression"); return nullptr; }
if (!activeExpr || !*activeExpr) { result=CR_INCOMPLETE; return nullptr; }
*activeExpr = parseExpression();
}}
return sw;
}

shared_ptr<Expression> QParser::parseSuper () {
shared_ptr<Expression> superExpr = make_shared<SuperExpression>(cur);
if (match(T_LEFT_PAREN)) {
auto expr = make_shared<NameExpression>(curMethodNameToken);
auto call = parseMethodCall(expr);
return BinaryOperation::create(superExpr, T_DOT, call);
}
consume(T_DOT, ("Expected '.' or '('  after 'super'"));
shared_ptr<Expression> expr = parseExpression();
return BinaryOperation::create(superExpr, T_DOT, expr);
}

shared_ptr<Expression> QParser::parseDebugExpression () {
shared_ptr<Expression> e = parseExpression();
return make_shared<DebugExpression>(e);
}

shared_ptr<Expression> QParser::parseUnpackOrExpression (int priority) {
if (match(T_DOTDOTDOT)) return make_shared<UnpackExpression>(parseExpression(priority));
else return parseExpression(priority);
}

shared_ptr<Expression> QParser::parseName () {
return make_shared<NameExpression>(cur);
}

shared_ptr<Expression> QParser::parseField () {
if (matchOneOf(T_COMMA, T_RIGHT_PAREN, T_RIGHT_BRACKET, T_RIGHT_BRACE, T_EQGT, T_MINUSGT, T_SEMICOLON, T_LINE)) {
prevToken();
cur.value = QV::UNDEFINED;
return make_shared<ConstantExpression>(cur);
}
consume(T_NAME, ("Expected field name after '_'"));
return make_shared<FieldExpression>(cur);
}

shared_ptr<Expression> QParser::parseStaticField () {
consume(T_NAME, ("Expected static field name after '@_'"));
return make_shared<StaticFieldExpression>(cur);
}

shared_ptr<Expression> QParser::parseGenericMethodSymbol () {
skipNewlines();
if (nextNameToken(true).type!=T_NAME) parseError("Expected method name after '::'");
return make_shared<GenericMethodSymbolExpression>(cur);
}

shared_ptr<Expression> QParser::parseLiteral () {
auto literal = make_shared<ConstantExpression>(cur);
if (cur.type==T_NUM && matchOneOf(T_NAME, T_LEFT_PAREN)) {
shared_ptr<Expression> expr = nullptr;
if (cur.type==T_NAME) {
prevToken();
expr = parseExpression(P_FACTOR);
}
else if (cur.type==T_LEFT_PAREN) expr = parseGroupOrTuple();
return BinaryOperation::create(expr, T_STAR, literal);
}
return literal;
}

shared_ptr<Expression> QParser::parseLiteralList () {
shared_ptr<LiteralListExpression> list = make_shared<LiteralListExpression>(cur);
if (!match(T_RIGHT_BRACKET)) {
do {
list->items.push_back(parseUnpackOrExpression());
} while (match(T_COMMA));
skipNewlines();
consume(T_RIGHT_BRACKET, ("Expected ']' to close list literal"));
}
return list;
}

shared_ptr<Expression> QParser::parseLiteralSet () {
shared_ptr<LiteralSetExpression> list = make_shared<LiteralSetExpression>(cur);
if (!match(T_GT)) {
do {
list->items.push_back(parseUnpackOrExpression(P_COMPARISON));
} while (match(T_COMMA));
skipNewlines();
consume(T_GT, ("Expected '>' to close set literal"));
}
return list;
}

shared_ptr<Expression> QParser::parseLiteralMap () {
shared_ptr<LiteralMapExpression> map = make_shared<LiteralMapExpression>(cur);
if (!match(T_RIGHT_BRACE)) {
do {
bool computed = false;
shared_ptr<Expression> key, value;
if (match(T_LEFT_BRACKET)) {
computed=true;
key =  parseExpression();
skipNewlines();
consume(T_RIGHT_BRACKET, ("Expected ']' to close computed map key"));
}
else key = parseUnpackOrExpression();
if (!match(T_COLON)) value = key;
else value = parseExpression();
if (!computed) key = nameExprToConstant(key);
map->items.push_back(make_pair(key, value));
} while (match(T_COMMA));
skipNewlines();
consume(T_RIGHT_BRACE, ("Expected '}' to close map literal"));
}
return map;
}

shared_ptr<Expression> QParser::parseLiteralGrid () {
QToken token = cur;
vector<vector<shared_ptr<Expression>>> data;
begin: do {
data.emplace_back();
auto& row = data.back();
do {
skipNewlines();
auto expr = parseExpression(P_BITWISE);
if (!expr) return nullptr;
row.push_back(expr);
} while(match(T_COMMA));
if (row.size() != data[0].size()) parseError("All rows must be of the same size (%d)", data[0].size());
if (match(T_SEMICOLON)) goto begin;
skipNewlines();
consume(T_BAR, "Expected '|' to close literal grid expression");
skipNewlines();
} while(match(T_BAR));
return make_shared<LiteralGridExpression>(token, data);
}

shared_ptr<Expression> QParser::parseLiteralRegex () {
string pattern, options;
while(*in && *in!='/') {
if (*in=='\n' || *in=='\r') { parseError("Unterminated regex literal"); return nullptr; }
if (*in=='\\' && in[1]=='/') ++in;
pattern.push_back(*in++);
}
while (*++in && ((*in>='a' && *in<='z') || (*in>='A' && *in<='Z'))) options.push_back(*in);
return make_shared<LiteralRegexExpression>(cur, pattern, options);
}

shared_ptr<Expression> QParser::parseGroupOrTuple () {
auto initial = cur;
if (match(T_RIGHT_PAREN)) return make_shared<LiteralTupleExpression>(initial, vector<shared_ptr<Expression>>() );
shared_ptr<Expression> expr = parseUnpackOrExpression();
bool isTuple = expr->isUnpack();
skipNewlines();
if (isTuple) consume(T_COMMA, "Expected ',' for single-item tuple");
else isTuple = match(T_COMMA);
if (isTuple) {
vector<shared_ptr<Expression>> items = { expr };
if (match(T_RIGHT_PAREN)) return make_shared<LiteralTupleExpression>(initial, items );
do {
items.push_back(parseUnpackOrExpression());
} while(match(T_COMMA));
consume(T_RIGHT_PAREN, ("Expected ')' to close tuple"));
return make_shared<LiteralTupleExpression>(initial, items);
}
else {
skipNewlines();
consume(T_RIGHT_PAREN, ("Expected ')' to close parenthesized expression"));
return expr;
}}

shared_ptr<Expression> QParser::parseExpression (int priority) {
skipNewlines();
if (priority == P_MEMBER)  nextNameToken(false); 
else nextToken();
const ParserRule* rule = &rules[cur.type];
if (!rule->prefix) {
parseError(("Expected expression"));
result = cur.type==T_END? CR_INCOMPLETE : CR_FAILED;
return nullptr;
}
shared_ptr<Expression> right, left = (this->*(rule->prefix))();
while(true){
rule = &rules[nextToken().type];
if (!rule->infix || priority>=rule->priority || !(right = (this->*(rule->infix))(left)) ) {
prevToken();
return left;
}
else left = right;
}}
