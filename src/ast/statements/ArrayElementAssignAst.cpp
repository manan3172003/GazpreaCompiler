#include "ast/statements/ArrayElementAssignAst.h"
#include "ast/Ast.h"

namespace gazprea::ast::statements {

NodeType ArrayElementAssignAst::getNodeType() const { return NodeType::ArrayElementAssign; }

std::string ArrayElementAssignAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "ArrayElementAssign: ";
  if (sym) {
    ss << sym->toString();
  }
  ss << "\n";
  ss << prefix + indent << "ArrayInstance:\n";
  ss << arrayInstance->toStringTree(prefix + indent + indent);
  ss << prefix + indent << "ElementIndex:\n";
  ss << elementIndex->toStringTree(prefix + indent + indent);
  return ss.str();
}

} // namespace gazprea::ast::statements