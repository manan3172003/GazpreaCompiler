#include "ast/expressions/TupleAccessAst.h"
#include "ast/Ast.h"

namespace gazprea::ast::expressions {

NodeType TupleAccessAst::getNodeType() const { return NodeType::TupleAccess; }

std::string TupleAccessAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "TupleAccess: " << tupleName << "." << fieldIndex << '\n';
  return ss.str();
}

} // namespace gazprea::ast::expressions