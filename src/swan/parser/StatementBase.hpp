#ifndef ___PARSER_COMPILER_STATEMENT_BASE1
#define ___PARSER_COMPILER_STATEMENT_BASE1
#include "Token.hpp"
#include "../../include/bitfield.hpp"
#include<memory>
#include<vector>

double strtod_c  (const char*, char** = nullptr);
double dlshift (double, double);
double drshift (double, double);
double dintdiv (double, double);

struct QCompiler;
struct TypeAnalyzer;

struct TypeInfo: std::enable_shared_from_this<TypeInfo> {
static std::shared_ptr<TypeInfo> ANY, MANY;
virtual bool isEmpty () { return false; }
virtual bool isNum () { return false; }
virtual bool isBool () { return false; }
virtual bool isString () { return false; }
virtual bool isFunction () { return false; }
virtual bool isNull  () { return false; }
virtual bool isUndefined () { return false; }
virtual bool isNullOrUndefined () { return isNull() || isUndefined(); }
virtual bool isExact () { return false; }
virtual bool isOptional () { return false; }
virtual std::shared_ptr<TypeInfo> resolve (TypeAnalyzer& ta) { return shared_from_this(); }
virtual std::shared_ptr<TypeInfo> merge (std::shared_ptr<TypeInfo> t, TypeAnalyzer& ta) = 0;
virtual std::string toString () = 0;
virtual std::string toBinString (QVM& vm) = 0;
virtual bool equals (const std::shared_ptr<TypeInfo>& other) = 0;
virtual ~TypeInfo () = default;
};

struct FunctionInfo {
virtual std::shared_ptr<TypeInfo> getReturnTypeInfo (int nPassedArgs=0, std::shared_ptr<TypeInfo>* passedArgs = nullptr) = 0;
virtual std::shared_ptr<TypeInfo> getArgTypeInfo (int n, int nPassedArgs = 0, std::shared_ptr<TypeInfo>* passedArgs = nullptr) = 0;
virtual int getArgCount () = 0;
virtual std::shared_ptr<TypeInfo> getFunctionTypeInfo (int nPassedArgs = 0, std::shared_ptr<TypeInfo>* passedArgs = nullptr) = 0;
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
virtual int analyze (TypeAnalyzer& ta) = 0;
virtual ~Statement () = default;
};

struct Expression: Statement {
std::shared_ptr<TypeInfo> type = TypeInfo::ANY;

bool isExpression () final override { return true; }
inline std::shared_ptr<Expression> shared_this () { return std::static_pointer_cast<Expression>(shared_from_this()); }
virtual std::shared_ptr<Expression> optimize () { return shared_this(); }
std::shared_ptr<Statement> optimizeStatement () override { return optimize(); }
virtual bool isComprehension () { return false; }
virtual bool isUnpack () { return false; }
};

struct Assignable {
virtual void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue) = 0;
virtual int analyzeAssignment (TypeAnalyzer& ta, std::shared_ptr<Expression> assignedValue) = 0;
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

bitfield(VarFlag, uint32_t){
None = 0,
Vararg = 1,
Pure = 2,
Final = 4,
Overridden = 8,
Accessor = 0x10,
Reserved1 = 0x20,
Reserved2 = 0x40,
Reserved3 = 0x80,
Method = 0x100,
Fiber = 0x200,
Async = 0x400,
Static = 0x800,
Const = 0x1000,
Global = 0x2000,
Export = 0x4000,
Single = 0x8000,
NoDefault = 0x10000,
Hoisted = 0x20000,
Inherited = 0x40000,
ReadOnly = 0x80000,
Optimized = 0x40000000,
};

struct Variable {
std::shared_ptr<Expression> name, value;
std::shared_ptr<TypeInfo> type;
std::vector<std::shared_ptr<Expression>> decorations;
bitmask<VarFlag> flags;

Variable (const std::shared_ptr<Expression>& nm, const std::shared_ptr<Expression>& val = nullptr, bitmask<VarFlag> flgs = VarFlag::None, const std::vector<std::shared_ptr<Expression>>& decos = {}):
name(nm), value(val), flags(flgs), decorations(decos), type(TypeInfo::ANY) {}
void optimize ();
};

QV doCompileTimeImport (QVM& vm, const std::string& baseFile, std::shared_ptr<Expression> exprRequestedFile);

std::shared_ptr<TypeInfo> getFunctionTypeInfo (FunctionInfo& fti, struct QVM& vm, int nArgs = 0, std::shared_ptr<TypeInfo>* ptr = nullptr);

#endif
