#include <ast/statements/DeclarationAst.h>

namespace gazprea::ast::statements {
NodeType DeclarationAst::getNodeType() const { return NodeType::Declaration; }
std::string DeclarationAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Declaration " << name << " " << qualifierToString(qualifier)
     << " ";
  if (type)
    type->toStringTree(prefix + indent);

  if (sym) {
    ss << sym->toString();
  }
  ss << '\n';
  if (expr) {
    ss << expr->toStringTree(prefix + indent);
  } else {
    ss << prefix + indent << "unknown" << '\n';
  }
  return ss.str();
}
} // namespace gazprea::ast::statements