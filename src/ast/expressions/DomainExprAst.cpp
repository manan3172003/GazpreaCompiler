#include "ast/expressions/DomainExprAst.h"

namespace gazprea::ast::expressions {

NodeType DomainExprAst::getNodeType() const { return NodeType::DomainExpr; }

std::string DomainExprAst::toStringTree(std::string prefix) const {
  std::string result = prefix + "DomainExpr: " + iteratorName + " in\n";
  if (domainExpression) {
    result += domainExpression->toStringTree(prefix + indent);
  }
  return result;
}

} // namespace gazprea::ast::expressions