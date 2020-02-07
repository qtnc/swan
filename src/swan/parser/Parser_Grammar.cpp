#include "Parser.hpp"
#include "ParserRules.hpp"
using namespace std;

unordered_map<int,ParserRule> rules = {
#define PREFIX(token, prefix, name) { T_##token, { &QParser::parse##prefix, nullptr, nullptr, nullptr, (#name), nullptr, P_PREFIX, P_LEFT  }}
#define PREFIX_OP(token, prefix, name) { T_##token, { &QParser::parse##prefix, nullptr, nullptr, &QParser::parseMethodDecl, (#name), nullptr, P_PREFIX, P_LEFT }}
#define INFIX(token, infix, name, priority, direction) { T_##token, { nullptr, &QParser::parse##infix, nullptr, nullptr, nullptr, (#name), P_##priority, P_##direction }}
#define INFIX_OP(token, infix, name, priority, direction) { T_##token, { nullptr, &QParser::parse##infix, nullptr, &QParser::parseMethodDecl, nullptr,  (#name), P_##priority, P_##direction  }}
#define MULTIFIX(token, prefix, infix, prefixName, infixName, priority, direction) { T_##token, { &QParser::parse##prefix, &QParser::parse##infix, nullptr, nullptr, (#prefixName), (#infixName), P_##priority, P_##direction }}
#define OPERATOR(token, prefix, infix, prefixName, infixName, priority, direction) { T_##token, { &QParser::parse##prefix, &QParser::parse##infix, nullptr, &QParser::parseMethodDecl, (#prefixName), (#infixName), P_##priority, P_##direction  }}
#define STATEMENT(token, func) { T_##token, { nullptr, nullptr, &QParser::parse##func, nullptr, nullptr, nullptr, P_PREFIX, P_LEFT  }}

OPERATOR(PLUS, PrefixOp, InfixOp, unp, +, TERM, LEFT),
OPERATOR(MINUS, PrefixOp, InfixOp, unm, -, TERM, LEFT),
INFIX_OP(STAR, InfixOp, *, FACTOR, LEFT),
INFIX_OP(BACKSLASH, InfixOp, \\, FACTOR, LEFT),
INFIX_OP(PERCENT, InfixOp, %, FACTOR, LEFT),
INFIX_OP(STARSTAR, InfixOp, **, EXPONENT, LEFT),
INFIX_OP(AMP, InfixOp, &, BITWISE, LEFT),
INFIX_OP(CIRC, InfixOp, ^, BITWISE, LEFT),
INFIX_OP(LTLT, InfixOp, <<, BITWISE, LEFT),
INFIX_OP(GTGT, InfixOp, >>, BITWISE, LEFT),
INFIX_OP(DOTDOT, InfixOp, .., RANGE, LEFT),
INFIX_OP(DOTDOTDOT, InfixOp, ..., RANGE, LEFT),
INFIX(AMPAMP, InfixOp, &&, LOGICAL, LEFT),
INFIX(BARBAR, InfixOp, ||, LOGICAL, LEFT),
INFIX(QUESTQUEST, InfixOp, ??, LOGICAL, LEFT),

INFIX(AS, TypeHint, as, ASSIGNMENT, RIGHT),
INFIX(EQ, InfixOp, =, ASSIGNMENT, RIGHT),
INFIX(PLUSEQ, InfixOp, +=, ASSIGNMENT, RIGHT),
INFIX(MINUSEQ, InfixOp, -=, ASSIGNMENT, RIGHT),
INFIX(STAREQ, InfixOp, *=, ASSIGNMENT, RIGHT),
INFIX(SLASHEQ, InfixOp, /=, ASSIGNMENT, RIGHT),
INFIX(BACKSLASHEQ, InfixOp, \\=, ASSIGNMENT, RIGHT),
INFIX(PERCENTEQ, InfixOp, %=, ASSIGNMENT, RIGHT),
INFIX(STARSTAREQ, InfixOp, **=, ASSIGNMENT, RIGHT),
INFIX(BAREQ, InfixOp, |=, ASSIGNMENT, RIGHT),
INFIX(AMPEQ, InfixOp, &=, ASSIGNMENT, RIGHT),
INFIX(CIRCEQ, InfixOp, ^=, ASSIGNMENT, RIGHT),
INFIX(ATEQ, InfixOp, @=, ASSIGNMENT, RIGHT),
INFIX(LTLTEQ, InfixOp, <<=, ASSIGNMENT, RIGHT),
INFIX(GTGTEQ, InfixOp, >>=, ASSIGNMENT, RIGHT),
INFIX(AMPAMPEQ, InfixOp, &&=, ASSIGNMENT, RIGHT),
INFIX(BARBAREQ, InfixOp, ||=, ASSIGNMENT, RIGHT),
INFIX(QUESTQUESTEQ, InfixOp, ?\x3F=, ASSIGNMENT, RIGHT),

INFIX_OP(EQEQ, InfixOp, ==, COMPARISON, LEFT),
INFIX_OP(EXCLEQ, InfixOp, !=, COMPARISON, LEFT),
OPERATOR(LT, LiteralSet, InfixOp, <, <, COMPARISON, LEFT),
INFIX_OP(GT, InfixOp, >, COMPARISON, LEFT),
INFIX_OP(LTE, InfixOp, <=, COMPARISON, LEFT),
INFIX_OP(GTE, InfixOp, >=, COMPARISON, LEFT),
INFIX_OP(LTEQGT, InfixOp, <=>, COMPARISON, LEFT),
INFIX_OP(IS, InfixIs, is, COMPARISON, SWAP_OPERANDS),
INFIX_OP(IN, InfixOp, in, COMPARISON, SWAP_OPERANDS),

INFIX(QUEST, Conditional, ?, CONDITIONAL, LEFT),
MULTIFIX(EXCL, PrefixOp, InfixNot, !, !, COMPARISON, LEFT),
PREFIX_OP(TILDE, PrefixOp, ~),
PREFIX(DOLLAR, DebugExpression, $),

#ifndef NO_REGEX
OPERATOR(SLASH, LiteralRegex, InfixOp, /, /, FACTOR, LEFT),
#else
INFIX_OP(SLASH, InfixOp, /, FACTOR, LEFT),
#endif
#ifndef NO_GRID
OPERATOR(BAR, LiteralGrid, InfixOp, |, |, BITWISE, LEFT),
#else
INFIX_OP(BAR, InfixOp, |, BITWISE, LEFT),
#endif

{ T_LEFT_PAREN, { &QParser::parseGroupOrTuple, &QParser::parseMethodCall, nullptr, &QParser::parseMethodDecl, nullptr, ("()"), P_CALL, P_LEFT }},
{ T_LEFT_BRACKET, { &QParser::parseLiteralList, &QParser::parseSubscript, nullptr, &QParser::parseMethodDecl, nullptr, ("[]"), P_SUBSCRIPT, P_LEFT }},
{ T_LEFT_BRACE, { &QParser::parseLiteralMap, nullptr, &QParser::parseBlock, nullptr, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_AT, { &QParser::parseDecoratedExpression, &QParser::parseInfixOp, &QParser::parseDecoratedStatement, &QParser::parseDecoratedDecl, "@", "@", P_EXPONENT, P_LEFT }},
{ T_FUNCTION, { &QParser::parseLambda, nullptr, &QParser::parseFunctionDecl, &QParser::parseMethodDecl2, "function", "function", P_PREFIX, P_LEFT }},
{ T_NAME, { &QParser::parseName, nullptr, nullptr, &QParser::parseMethodDecl, nullptr, nullptr, P_PREFIX, P_LEFT }},
INFIX(MINUSGT, ArrowFunction, ->, COMPREHENSION, RIGHT),
INFIX(EQGT, ArrowFunction, =>, COMPREHENSION, RIGHT),
INFIX(DOT, InfixOp, ., MEMBER, LEFT),
INFIX(DOTQUEST, InfixOp, .?, MEMBER, LEFT),
MULTIFIX(COLONCOLON, GenericMethodSymbol, InfixOp, ::, ::, MEMBER, LEFT),
PREFIX(UND, Field, _),
PREFIX(UNDUND, StaticField, __),
PREFIX(SUPER, Super, super),
PREFIX(TRUE, Literal, true),
PREFIX(FALSE, Literal, false),
PREFIX(NULL, Literal, null),
PREFIX(UNDEFINED, Literal, undefined),
PREFIX(NUM, Literal, Num),
PREFIX(STRING, Literal, String),
PREFIX(YIELD, Yield, yield),
PREFIX(AWAIT, Yield, await),

STATEMENT(SEMICOLON, SimpleStatement),
STATEMENT(BREAK, Break),
STATEMENT(CLASS, ClassDecl),
STATEMENT(CONTINUE, Continue),
STATEMENT(EXPORT, ExportDecl),
STATEMENT(GLOBAL, GlobalDecl),
STATEMENT(IF, If),
STATEMENT(REPEAT, RepeatWhile),
STATEMENT(RETURN, Return),
STATEMENT(THROW, Throw),
STATEMENT(TRY, Try),
STATEMENT(WITH, With),
STATEMENT(WHILE, While),

{ T_FOR, { nullptr, &QParser::parseComprehension, &QParser::parseFor, nullptr, nullptr, "for", P_COMPREHENSION, P_LEFT }},
{ T_IMPORT, { &QParser::parseImportExpression, nullptr, &QParser::parseImportDecl, nullptr, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_VAR, { nullptr, nullptr, &QParser::parseVarDecl, &QParser::parseSimpleAccessor, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_CONST, { nullptr, nullptr, &QParser::parseVarDecl, &QParser::parseSimpleAccessor, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_SWITCH, { &QParser::parseSwitchExpression, nullptr, &QParser::parseSwitchStatement, nullptr, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_ASYNC, { nullptr, nullptr, &QParser::parseAsync, &QParser::parseAsyncMethodDecl, nullptr, nullptr, P_PREFIX, P_LEFT }}
#undef PREFIX
#undef PREFIX_OP
#undef INFIX
#undef INFIX_OP
#undef OPERATOR
#undef STATEMENT
};

unordered_map<string,QTokenType> KEYWORDS = {
#define TOKEN(name, keyword) { (#keyword), T_##name }
TOKEN(AMPAMP, and),
TOKEN(AS, as),
TOKEN(ASYNC, async),
TOKEN(AWAIT, await),
TOKEN(BREAK, break),
TOKEN(CASE, case),
TOKEN(CATCH, catch),
TOKEN(CLASS, class),
TOKEN(CONTINUE, continue),
TOKEN(CONST, const),
TOKEN(FUNCTION, def),
TOKEN(DEFAULT, default),
TOKEN(ELSE, else),
TOKEN(EXPORT, export),
TOKEN(FALSE, false),
TOKEN(FINALLY, finally),
TOKEN(FOR, for),
TOKEN(FUNCTION, function),
TOKEN(GLOBAL, global),
TOKEN(IF, if),
TOKEN(IMPORT, import),
TOKEN(IN, in),
TOKEN(IS, is),
TOKEN(VAR, let),
TOKEN(EXCL, not),
TOKEN(NULL, null),
TOKEN(BARBAR, or),
TOKEN(REPEAT, repeat),
TOKEN(RETURN, return),
TOKEN(STATIC, static),
TOKEN(SUPER, super),
TOKEN(SWITCH, switch),
TOKEN(THROW, throw),
TOKEN(TRUE, true),
TOKEN(TRY, try),
TOKEN(UNDEFINED, undefined),
TOKEN(VAR, var),
TOKEN(WHILE, while),
TOKEN(WITH, with),
TOKEN(YIELD, yield)
#undef TOKEN
};

const char* TOKEN_NAMES[] = {
#define TOKEN(name) "T_" #name
#include "TokenTypes.hpp"
#undef TOKEN
};

const char 
*THIS = "this",
*EXPORTS = "exports",
*FIBER = "Fiber",
*ASYNC = "async",
*CONSTRUCTOR = "constructor";
