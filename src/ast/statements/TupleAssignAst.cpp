#include "ast/statements/TupleAssignAst.h"
#include "ast/Ast.h"

namespace gazprea::ast::statements {

NodeType TupleAssignAst::getNodeType() const { return NodeType::TupleAssign; }

std::string TupleAssignAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Tuple Field: " << tupleName << "." << fieldIndex
     << scopeToString() << '\n';
  return ss.str();
}

} // namespace gazprea::ast::statements