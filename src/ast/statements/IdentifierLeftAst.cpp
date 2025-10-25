#include "ast/statements/IdentifierLeftAst.h"
#include "ast/Ast.h"

namespace gazprea::ast::statements {

NodeType IdentifierLeftAst::getNodeType() const {
  return NodeType::IdentifierLeft;
}
std::string IdentifierLeftAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Id: " << name << "\n";
  return ss.str();
}
} // namespace gazprea::ast::statements