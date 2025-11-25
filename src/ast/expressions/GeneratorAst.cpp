#include "ast/expressions/GeneratorAst.h"

namespace gazprea::ast::expressions {

NodeType GeneratorAst::getNodeType() const { return NodeType::Generator; }

std::string GeneratorAst::toStringTree(std::string prefix) const {
  std::string result = prefix + "Generator (" + std::to_string(domainExprs.size()) + "D):\n";

  for (size_t i = 0; i < domainExprs.size(); ++i) {
    result += prefix + indent + "Domain" + std::to_string(i + 1) + ":\n";
    if (domainExprs[i]) {
      result += domainExprs[i]->toStringTree(prefix + indent + indent);
    }
  }

  if (generatorExpression) {
    result += prefix + indent + "GeneratorExpr:\n";
    result += generatorExpression->toStringTree(prefix + indent + indent);
  }

  return result;
}

} // namespace gazprea::ast::expressions