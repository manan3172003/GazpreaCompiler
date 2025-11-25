#include "ast/expressions/ArrayAccessAst.h"
#include "ast/Ast.h"

namespace gazprea::ast::expressions {

NodeType ArrayAccessAst::getNodeType() const { return NodeType::ArrayAccess; }

std::string ArrayAccessAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "ArrayAccess: ";
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

} // namespace gazprea::ast::expressions