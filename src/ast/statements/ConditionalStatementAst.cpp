#include <ast/statements/ConditionalStatementAst.h>

namespace gazprea::ast::statements {
NodeType ConditionalStatementAst::getNodeType() const {
  return NodeType::ConditionalStatement;
}
std::string ConditionalStatementAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "IfStatement\n";

  ss << prefix << ". . Condition\n";
  if (condition) {
    ss << condition->toStringTree(prefix + indent);
  }

  ss << prefix << ". . Then\n";
  if (thenBody) {
    ss << thenBody->toStringTree(prefix + indent);
  }

  if (elseBody) {
    ss << prefix << ". . Else\n";
    ss << elseBody->toStringTree(prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::statements