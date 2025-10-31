#include "ast/statements/TupleElementAssignAst.h"
#include "ast/Ast.h"

namespace gazprea::ast::statements {

NodeType TupleElementAssignAst::getNodeType() const { return NodeType::TupleElementAssign; }

std::string TupleElementAssignAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Tuple Field: " << tupleName << "." << fieldIndex << scopeToString();
  if (sym) {
    ss << sym->toString();
  }
  ss << "\n";
  return ss.str();
}

} // namespace gazprea::ast::statements