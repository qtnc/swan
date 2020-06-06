#include "Expression.hpp"
#include "Constants.hpp"
#include<cstring>
using namespace std;


shared_ptr<FunctionDeclaration> ClassDeclaration::findMethod (const QToken& name, bool isStatic) {
auto it = find_if(methods.begin(), methods.end(), [&](auto& m){ 
return m->name.length==name.length && strncmp(name.start, m->name.start, name.length)==0
&& ((!!static_cast<bool>(m->flags & VarFlag::Static))==isStatic);
});
return it==methods.end()? nullptr : *it;
}
