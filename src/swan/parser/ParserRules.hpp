#ifndef ___PARSER_COMPILER_RULES1
#define ___PARSER_COMPILER_RULES1
#include "Parser.hpp"
#include "Token.hpp"
#include<unordered_map>

struct ParserRule {
typedef std::shared_ptr<Expression>(QParser::*PrefixFn)(void);
typedef std::shared_ptr<Expression>(QParser::*InfixFn)(std::shared_ptr<Expression> left);
typedef std::shared_ptr<Statement>(QParser::*StatementFn)(void);
typedef void(QParser::*MemberFn)(ClassDeclaration&,int);

PrefixFn prefix = nullptr;
InfixFn infix = nullptr;
StatementFn statement = nullptr;
MemberFn member = nullptr;
const char* prefixOpName = nullptr;
const char* infixOpName = nullptr;
uint8_t priority = P_HIGHEST;
uint8_t flags = 0;
};

extern std::unordered_map<int,ParserRule> rules;

extern const char  *THIS, *EXPORTS, *FIBER, *ASYNC, *CONSTRUCTOR; 

#endif
