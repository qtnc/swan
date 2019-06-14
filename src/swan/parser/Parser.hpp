#ifndef _____PARSER_HPP_____
#define _____PARSER_HPP_____
#include "Constants.hpp"
#include "Token.hpp"
#include<string>
#include<memory>

struct Expression;
struct Statement;

struct QParser  {
const char *in, *start, *end;
std::string filename, displayName;
QToken cur, prev;
QToken curMethodNameToken = { T_END, "#", 1,  QV() };
std::vector<QToken> stackedTokens;
QVM& vm;
CompilationResult result = CR_SUCCESS;

QParser (QVM& vm0, const std::string& source, const std::string& filename0, const std::string& displayName0): vm(vm0), in(source.data()), start(source.data()), end(source.data() + source.length()), filename(filename0), displayName(displayName0)  {}

const QToken& nextToken ();
const QToken& nextNameToken (bool);
const QToken& prevToken();
QToken createTempName ();
std::pair<int,int> getPositionOf (const char*);
template<class... A> void parseError (const char* fmt, const A&... args);

void skipNewlines ();
bool match (QTokenType type);
bool consume (QTokenType type, const char* msg);
template<class... T> bool matchOneOf (T... tokens);

std::shared_ptr<Expression> parseExpression (int priority = P_LOWEST);
std::shared_ptr<Expression> parsePrefixOp ();
std::shared_ptr<Expression> parseInfixOp (std::shared_ptr<Expression> left);
std::shared_ptr<Expression> parseConditional  (std::shared_ptr<Expression> condition);
std::shared_ptr<Expression> parseComprehension   (std::shared_ptr<Expression> body);
std::shared_ptr<Expression> parseMethodCall (std::shared_ptr<Expression> receiver);
std::shared_ptr<Expression> parseSubscript (std::shared_ptr<Expression> receiver);
std::shared_ptr<Expression> parseGroupOrTuple ();
std::shared_ptr<Expression> parseLiteral ();
std::shared_ptr<Expression> parseLiteralList ();
std::shared_ptr<Expression> parseLiteralSet ();
std::shared_ptr<Expression> parseLiteralMap ();
std::shared_ptr<Expression> parseLiteralGrid ();
std::shared_ptr<Expression> parseLiteralRegex ();
std::shared_ptr<Expression> parseName ();
std::shared_ptr<Expression> parseField ();
std::shared_ptr<Expression> parseStaticField  ();
std::shared_ptr<Expression> parseLambda ();
std::shared_ptr<Expression> parseSuper ();
std::shared_ptr<Expression> parseUnpackOrExpression (int priority = P_LOWEST);
std::shared_ptr<Expression> parseGenericMethodSymbol ();
std::shared_ptr<Expression> parseDecoratedExpression ();
void parseFunctionParameters (std::shared_ptr<struct FunctionDeclaration>& func);

std::shared_ptr<Statement> parseStatement ();
std::shared_ptr<Statement> parseStatements ();
std::shared_ptr<Statement> parseSimpleStatement ();
std::shared_ptr<Statement> parseDecoratedStatement ();
std::shared_ptr<Statement> parseBlock ();
std::shared_ptr<Statement> parseIf ();
std::shared_ptr<Statement> parseSwitchStatement ();
std::shared_ptr<Expression> parseSwitchExpression ();
std::shared_ptr<Statement> parseFor ();
std::shared_ptr<Statement> parseWhile ();
std::shared_ptr<Statement> parseRepeatWhile ();
std::shared_ptr<Statement> parseContinue ();
std::shared_ptr<Statement> parseBreak ();
std::shared_ptr<Statement> parseReturn ();
std::shared_ptr<Statement> parseTry ();
std::shared_ptr<Statement> parseThrow ();
std::shared_ptr<Statement> parseWith ();
std::shared_ptr<Expression> parseYield ();
std::shared_ptr<Statement> parseVarDecl ();
std::shared_ptr<Statement> parseVarDecl (int flags);
void parseVarList (std::vector<std::shared_ptr<struct Variable>>& vars, int flags = 0);
std::shared_ptr<Statement> parseExportDecl ();
std::shared_ptr<Statement> parseImportDecl (bool expressionOnly);
std::shared_ptr<Statement> parseImportDecl ();
std::shared_ptr<Expression> parseImportExpression ();
std::shared_ptr<Statement> parseGlobalDecl ();
std::shared_ptr<Statement> parseClassDecl (int flags);
std::shared_ptr<Statement> parseClassDecl ();
std::shared_ptr<Statement> parseFunctionDecl (int flags);
std::shared_ptr<Statement> parseFunctionDecl ();

void parseMethodDecl (struct ClassDeclaration&, bool);
void parseSimpleAccessor (struct ClassDeclaration&, bool);
void parseDecoratedDecl (struct ClassDeclaration&, bool);
};

#endif
