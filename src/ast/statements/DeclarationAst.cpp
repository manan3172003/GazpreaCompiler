#include <ast/statements/DeclarationAst.h>

namespace gazprea::ast::statements {
NodeType DeclarationAst::getNodeType() const { return NodeType::Declaration; }
std::string DeclarationAst::toStringTree() const {
  std::stringstream ss;
  ss << "Declaration " << name << " " << qualifierToString(qualifier) << " "
     << type << " " << token->toString() << '\n';
  return ss.str();
}
} // namespace gazprea::ast::statements