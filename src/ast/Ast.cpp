#include "Colors.h"

#include <ast/Ast.h>
namespace gazprea::ast {
std::string Ast::qualifierToString(Qualifier qualifier) {
  switch (qualifier) {
  case Qualifier::Var:
    return "var";
  case Qualifier::Const:
    return "const";
  }
  return "";
}
std::string Ast::scopeToString() const {
  std::stringstream ss;
  if (scope) {
    ss << " (Scope: " << KYEL << scope << RST << ")";
  } else {
    ss << " (Scope: " << KRED << "undefined" << RST << ")";
  }
  return ss.str();
}
} // namespace gazprea::ast