#ifndef ___PARSER_COMPILER_STATEMENT_BASE1
#define ___PARSER_COMPILER_STATEMENT_BASE1
#include "Token.hpp"
#include<memory>
#include<vector>

double strtod_c  (const char*, char** = nullptr);
double dlshift (double, double);
double drshift (double, double);
double dintdiv (double, double);

struct QCompiler;

struct TypeInfo: std::enable_shared_from_this<TypeInfo> {
static std::shared_ptr<TypeInfo> ANY, MANY;
virtual bool isEmpty () { return false; }
virtual bool isNum (QVM& vm) { return false; }
virtual bool isBool (QVM& vm) { return false; }
virtual std::shared_ptr<TypeInfo> resolve (QCompiler& compiler) { return shared_from_this(); }
virtual std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, QCompiler& compiler) = 0;
virtual std::string toString () = 0;
virtual std::string toBinString (QVM& vm) = 0;
virtual ~TypeInfo () = default;
};

struct FunctionInfo {
virtual std::shared_ptr<TypeInfo> getReturnTypeInfo (int nArgs=0, std::shared_ptr<TypeInfo>* ptr = nullptr) = 0;
virtual std::shared_ptr<TypeInfo> getArgTypeInfo (int n) = 0;
virtual int getArgCount () = 0;
virtual std::shared_ptr<TypeInfo> getFunctionTypeInfo (int nArgs = 0, std::shared_ptr<TypeInfo>* ptr = nullptr) = 0;
virtual int getFlags () { return 0; }
virtual int getFieldIndex () { return -1; }
virtual ~FunctionInfo () = default;
};

struct Statement: std::enable_shared_from_this<Statement>  {
inline std::shared_ptr<Statement> shared_this () { return shared_from_this(); }
virtual const QToken& nearestToken () = 0;
virtual bool isExpression () { return false; }
virtual bool isDecorable () { return false; }
virtual bool isUsingExports () { return false; }
virtual std::shared_ptr<Statement> optimizeStatement () { return shared_this(); }
virtual void compile (QCompiler& compiler) = 0;
virtual ~Statement () = default;
};

struct Expression: Statement {
bool isExpression () final override { return true; }
inline std::shared_ptr<Expression> shared_this () { return std::static_pointer_cast<Expression>(shared_from_this()); }
virtual std::shared_ptr<Expression> optimize () { return shared_this(); }
std::shared_ptr<Statement> optimizeStatement () override { return optimize(); }
virtual std::shared_ptr<TypeInfo> getType (QCompiler& compiler) = 0;
virtual bool isComprehension () { return false; }
virtual bool isUnpack () { return false; }
};

struct Assignable {
virtual void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue) = 0;
virtual bool isAssignable () { return true; }
};

struct Decorable {
std::vector<std::shared_ptr<Expression>> decorations;
virtual bool isDecorable () { return true; }
};

struct Comprenable {
virtual void chain (const std::shared_ptr<Statement>& sta) = 0;
};

struct Functionnable {
virtual void makeFunctionParameters (std::vector<std::shared_ptr<struct Variable>>& params) = 0;
virtual bool isFunctionnable () = 0;
};

struct Variable {
std::shared_ptr<Expression> name, value;
std::shared_ptr<TypeInfo> typeHint;
std::vector<std::shared_ptr<Expression>> decorations;
int flags;

Variable (const std::shared_ptr<Expression>& nm, const std::shared_ptr<Expression>& val = nullptr, int flgs = 0, const std::vector<std::shared_ptr<Expression>>& decos = {}):
name(nm), value(val), flags(flgs), decorations(decos), typeHint(nullptr) {}
void optimize ();
};

void doCompileTimeImport (QVM& vm, const std::string& baseFile, std::shared_ptr<Expression> exprRequestedFile);

std::shared_ptr<TypeInfo> getFunctionTypeInfo (FunctionInfo& fti, struct QVM& vm, int nArgs = 0, std::shared_ptr<TypeInfo>* ptr = nullptr);

#endif
