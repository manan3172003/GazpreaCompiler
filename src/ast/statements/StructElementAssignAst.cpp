#include "ast/statements/StructElementAssignAst.h"
#include "ast/Ast.h"

namespace gazprea::ast::statements {

NodeType StructElementAssignAst::getNodeType() const { return NodeType::StructElementAssign; }

std::string StructElementAssignAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "StructElement: " << structName << "." << elementName << scopeToString();
  if (sym) {
    ss << sym->toString();
  }
  ss << "\n";
  return ss.str();
}

} // namespace gazprea::ast::statements