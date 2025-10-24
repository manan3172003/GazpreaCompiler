#include <ast/expressions/IdentifierAst.h>

namespace gazprea::ast::expressions {
NodeType IdentifierAst::getNodeType() const { return NodeType::Identifier; }
std::string IdentifierAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Identifier: " << name << "\n";
  return ss.str();
}
} // namespace gazprea::ast::expressions