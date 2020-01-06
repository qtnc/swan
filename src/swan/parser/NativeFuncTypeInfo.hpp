#ifndef ___COMPILER_PARSER_TYPE_INFONATIVE0
#define ___COMPILER_PARSER_TYPE_INFONATIVE0
#include "StatementBase.hpp"

void nativeFuncTypeInfoDeleter (struct NativeFuncTypeInfo* nfti);

struct QVM;
struct QClass;

struct NativeFuncTypeInfo {
typedef std::function< std::shared_ptr<TypeInfo>(QVM&,int,std::shared_ptr<TypeInfo>*) > RTFunc;

QClass* returnType;
std::unique_ptr<QClass*[]> argtypes;
int nArgs;
bool vararg;
RTFunc rtFromArgs;

NativeFuncTypeInfo (int n, QClass** clss, QClass* rt, bool va=false, const RTFunc& f=nullptr):
nArgs(n), vararg(va), returnType(rt), rtFromArgs(f),
argtypes(std::make_unique<QClass*[]>(n)) 
{ std::copy(clss, clss+n, &argtypes[0]); }
NativeFuncTypeInfo (const std::initializer_list<QClass*>& il, QClass* rt, bool va=false, const RTFunc& f=nullptr):
nArgs(il.size()), vararg(va), returnType(rt), rtFromArgs(f),
argtypes(std::make_unique<QClass*[]>(il.size())) 
{ std::copy(il.begin(), il.end(), &argtypes[0]); }
static NativeFuncTypeInfo* create  (const std::initializer_list<QClass*>& il, QClass* rt, bool va=false, const RTFunc& f=nullptr) { return new NativeFuncTypeInfo(il, rt, va, f); }

std::shared_ptr<struct TypeInfo> getArgType (int n) ;
std::shared_ptr<struct TypeInfo> getReturnType () ;
std::shared_ptr<struct TypeInfo> getFunctionType (QVM& vm);
std::shared_ptr<struct TypeInfo> getReturnType (QVM& vm, int nArgs, std::shared_ptr<TypeInfo>* args);
};

#endif
