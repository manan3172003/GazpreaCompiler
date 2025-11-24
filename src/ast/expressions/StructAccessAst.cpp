#include "ast/expressions/StructAccessAst.h"
#include "ast/Ast.h"

namespace gazprea::ast::expressions {

NodeType StructAccessAst::getNodeType() const { return NodeType::StructAccess; }

std::string StructAccessAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "StructAccess: " << structName << "." << elementName;
  if (sym) {
    ss << sym->toString();
  }
  ss << "\n";
  return ss.str();
}

} // namespace gazprea::ast::expressions