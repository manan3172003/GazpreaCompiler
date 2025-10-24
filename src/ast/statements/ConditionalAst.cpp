#include <ast/statements/ConditionalAst.h>

namespace gazprea::ast::statements {
NodeType ConditionalStatementAst::getNodeType() const {
  return NodeType::ConditionalStatement;
}
std::string ConditionalStatementAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "IfStatement: \n";

  ss << prefix + indent << "Condition: \n";
  if (condition) {
    ss << condition->toStringTree(indent + prefix + indent);
  }

  ss << prefix + indent << "Then: \n";
  if (thenBody) {
    ss << thenBody->toStringTree(indent + prefix + indent);
  }

  if (elseBody) {
    ss << prefix + indent << "Else: \n";
    ss << elseBody->toStringTree(indent + prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::statements