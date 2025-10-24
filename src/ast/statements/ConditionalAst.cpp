#include <ast/statements/ConditionalAst.h>

namespace gazprea::ast::statements {

void ConditionalAst::setCondition(
    std::shared_ptr<expressions::ExpressionAst> condition) {
  this->condition = condition;
}
void ConditionalAst::setThenBody(std::shared_ptr<BlockAst> thenBody) {
  this->thenBody = thenBody;
}
void ConditionalAst::setElseBody(std::shared_ptr<BlockAst> elseBody) {
  this->elseBody = elseBody;
}

std::shared_ptr<BlockAst> ConditionalAst::getThenBody() const {
  return this->thenBody;
}
std::shared_ptr<BlockAst> ConditionalAst::getElseBody() const {
  return this->elseBody;
}
std::shared_ptr<expressions::ExpressionAst>
ConditionalAst::getCondition() const {
  return this->condition;
}

NodeType ConditionalAst::getNodeType() const { return NodeType::Conditional; }
std::string ConditionalAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "IfStatement: \n";

  ss << prefix + indent << "Condition: \n";
  if (getCondition()) {
    ss << getCondition()->toStringTree(indent + prefix + indent);
  }

  ss << prefix + indent << "Then: \n";
  if (getThenBody()) {
    ss << getThenBody()->toStringTree(indent + prefix + indent);
  }

  if (getElseBody()) {
    ss << prefix + indent << "Else: \n";
    ss << getElseBody()->toStringTree(indent + prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::statements