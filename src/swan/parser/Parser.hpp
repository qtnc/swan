#ifndef _____PARSER_HPP_____
#define _____PARSER_HPP_____
#include "Constants.hpp"
#include "Token.hpp"
#include "../../include/cpprintf.hpp"
#include<string>
#include<memory>

struct Expression;
struct Statement;
struct Variable;

struct QParser  {
const char *in, *start, *end;
std::string filename, displayName;
QToken cur, prev;
QToken curMethodNameToken = { T_END, "#", 1, QV::UNDEFINED  };
std::vector<QToken> stackedTokens;
QVM& vm;
CompilationResult result = CR_SUCCESS;

QParser (QVM& vm0, const std::string& source, const std::string& filename0, const std::string& displayName0): vm(vm0), in(source.data()), start(source.data()), end(source.data() + source.length()), filename(filename0), displayName(displayName0)  {}

const QToken& nextToken ();
const QToken& nextNameToken (bool);
const QToken& prevToken();
QToken createTempName ();
std::pair<int,int> getPositionOf (const char*);

void printMessage (const QToken& tok, Swan::CompilationMessage::Kind msgtype, const std::string& msg);
template<class... A> inline void parseError (const char* fmt, const A&... args) { printMessage(cur, Swan::CompilationMessage::Kind::ERROR, format(fmt, args...)); result = cur.type==T_END? CR_INCOMPLETE : CR_FAILED; }

void skipNewlines ();
bool match (QTokenType type);
bool consume (QTokenType type, const char* msg);
template<class... T> bool matchOneOf (T... tokens);

std::shared_ptr<Expression> parseExpression (int priority = P_LOWEST);
std::shared_ptr<Expression> parsePrefixOp ();
std::shared_ptr<Expression> parseInfixOp (std::shared_ptr<Expression> left);
std::shared_ptr<Expression> parseInfixIs (std::shared_ptr<Expression> left);
std::shared_ptr<Expression> parseInfixNot (std::shared_ptr<Expression> left);
std::shared_ptr<Expression> parseTypeHint (std::shared_ptr<Expression> left);
std::shared_ptr<Expression> parseConditional  (std::shared_ptr<Expression> condition);
std::shared_ptr<Expression> parseComprehension   (std::shared_ptr<Expression> body);
std::shared_ptr<Expression> parseMethodCall (std::shared_ptr<Expression> receiver);
std::shared_ptr<Expression> parseSubscript (std::shared_ptr<Expression> receiver);
std::shared_ptr<Expression> parseGroupOrTuple ();
std::shared_ptr<Expression> parseLiteral ();
std::shared_ptr<Expression> parseLiteralList ();
std::shared_ptr<Expression> parseLiteralMapOrSet ();
std::shared_ptr<Expression> parseLiteralGrid (QToken, std::vector<std::shared_ptr<Expression>>&);
std::shared_ptr<Expression> parseLiteralRegex ();
std::shared_ptr<Expression> parseName ();
std::shared_ptr<Expression> parseField ();
std::shared_ptr<Expression> parseStaticField  ();
std::shared_ptr<Expression> parseLambda ();
std::shared_ptr<Expression> parseLambda (int flags);
std::shared_ptr<Expression> parseArrowFunction (std::shared_ptr<Expression> argexpr);
std::shared_ptr<Expression> parseSuper ();
std::shared_ptr<Expression> parseUnpackOrExpression (int priority = P_LOWEST);
std::shared_ptr<Expression> parseGenericMethodSymbol ();
std::shared_ptr<Expression> parseDebugExpression ();
std::shared_ptr<Expression> parseDecoratedExpression ();
void parseFunctionParameters (std::shared_ptr<struct FunctionDeclaration>& func, struct ClassDeclaration* cld = nullptr);

std::shared_ptr<Statement> parseStatement ();
std::shared_ptr<Statement> parseStatements ();
std::shared_ptr<Statement> parseSimpleStatement ();
std::shared_ptr<Statement> parseDecoratedStatement ();
std::shared_ptr<Statement> parseBlock ();
std::shared_ptr<Statement> parseIf ();
std::shared_ptr<Statement> parseSwitchStatement ();
std::shared_ptr<Expression> parseSwitchExpression ();
std::shared_ptr<Expression> parseSwitchCase (std::shared_ptr<Expression> left);
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
void parseVarList (std::vector<std::shared_ptr<Variable>>& vars, int flags = 0);
std::shared_ptr<Statement> parseExportDecl ();
std::shared_ptr<Statement> parseImportDecl (bool expressionOnly);
std::shared_ptr<Statement> parseImportDecl ();
std::shared_ptr<Statement> parseImportDecl2 ();
std::shared_ptr<Expression> parseImportExpression ();
std::shared_ptr<Statement> parseGlobalDecl ();
std::shared_ptr<Statement> parseClassDecl (int flags);
std::shared_ptr<Statement> parseClassDecl ();
std::shared_ptr<Statement> parseFunctionDecl (int varFlags, int funcFlags=0);
std::shared_ptr<Statement> parseFunctionDecl ();
std::shared_ptr<Statement> parseAsyncFunctionDecl (int varFlags);
std::shared_ptr<Statement> parseAsync ();

void parseMethodDecl (struct ClassDeclaration&, int);
void parseSimpleAccessor (struct ClassDeclaration&, int);
void parseDecoratedDecl (struct ClassDeclaration&, int);
void parseAsyncMethodDecl (struct ClassDeclaration&, int);
void parseMethodDecl2 (struct ClassDeclaration&, int);

std::shared_ptr<Expression> nameExprToConstant (std::shared_ptr<Expression> key);
void multiVarExprToSingleLiteralMap (std::vector<std::shared_ptr<Variable>>& vars, int flags);

std::shared_ptr<struct TypeInfo> parseTypeInfo ();
};

template<class... T> bool QParser::matchOneOf (T... tokens) {
std::vector<QTokenType> vt = { tokens... };
nextToken();
if (vt.end()!=std::find(vt.begin(), vt.end(), cur.type)) return true;
prevToken();
return false;
}

inline bool isSpace (uint32_t c) {
return c==' ' || c=='\t' || c=='\r' || c==160;
}

inline bool isLine (uint32_t c) {
return c=='\n';
}

bool isDigit (uint32_t c);

#endif
