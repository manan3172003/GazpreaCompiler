#include "ast/statements/LoopAst.h"

namespace gazprea::ast::statements {

void LoopAst::setBody(std::shared_ptr<BlockAst> bodyBlock) { this->body = bodyBlock; }

void LoopAst::setCondition(std::shared_ptr<expressions::ExpressionAst> cond) {
  this->condition = cond;
}
void LoopAst::setIsPostPredicated(bool isPost) { this->isPostPredicated = isPost; }

std::shared_ptr<BlockAst> LoopAst::getBody() const { return this->body; }

std::shared_ptr<expressions::ExpressionAst> LoopAst::getCondition() const {
  return this->condition;
}

bool LoopAst::getIsPostPredicated() const { return this->isPostPredicated; }

NodeType LoopAst::getNodeType() const { return NodeType::Loop; }
std::string LoopAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Loop: \n";

  if (condition) {
    ss << prefix << indent << "Condition: \n";
    ss << condition->toStringTree(prefix + indent + indent);
  }

  ss << prefix << indent << "Body: \n";
  if (body) {
    ss << body->toStringTree(prefix + indent + indent);
  }

  return ss.str();
}
} // namespace gazprea::ast::statements