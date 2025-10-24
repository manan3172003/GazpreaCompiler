#include <ast/statements/DeclarationAst.h>

namespace gazprea::ast::statements {
NodeType DeclarationAst::getNodeType() const { return NodeType::Declaration; }
std::string DeclarationAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Declaration " << name << " " << qualifierToString(qualifier)
     << " " << type << '\n';
  ss << expr->toStringTree(prefix + "----");
  return ss.str();
}
} // namespace gazprea::ast::statements