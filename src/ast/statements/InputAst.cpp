#include <ast/statements/InputAst.h>

namespace gazprea::ast::statements {
NodeType InputAst::getNodeType() const { return NodeType::Input; }
std::string InputAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Input(" << this->identifier << ")";
  if (sym) {
    ss << " (Symbol: " << sym->toString() << ")";
  }
  ss << "\n";
  return ss.str();
}
std::string InputAst::getIdentifier() const { return identifier; }
void InputAst::setIdentifier(const std::string &identifier) {
  this->identifier = identifier;
}
} // namespace gazprea::ast::statements