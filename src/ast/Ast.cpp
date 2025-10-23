#include <ast/Ast.h>
namespace gazprea::ast {
std::string Ast::qualifierToString(Qualifier qualifier) {
  switch (qualifier) {
  case Qualifier::Var:
    return "var";
  case Qualifier::Const:
    return "const";
  }
}
} // namespace gazprea::ast