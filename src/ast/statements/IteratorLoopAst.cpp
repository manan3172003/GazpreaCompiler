#include "ast/statements/IteratorLoopAst.h"

namespace gazprea::ast::statements {

void IteratorLoopAst::setBody(std::shared_ptr<BlockAst> bodyBlock) { this->body = bodyBlock; }

void IteratorLoopAst::setIteratorName(std::string name) { this->iteratorName = name; }

void IteratorLoopAst::setDomainExpr(std::shared_ptr<expressions::ExpressionAst> expr) {
  this->domainExpr = expr;
}

std::shared_ptr<BlockAst> IteratorLoopAst::getBody() const { return this->body; }

std::string IteratorLoopAst::getIteratorName() const { return this->iteratorName; }

std::shared_ptr<expressions::ExpressionAst> IteratorLoopAst::getDomainExpr() const {
  return this->domainExpr;
}

NodeType IteratorLoopAst::getNodeType() const { return NodeType::IteratorLoop; }

std::string IteratorLoopAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "IteratorLoop: \n";
  ss << prefix << indent << "Iterator: " << iteratorName << "\n";
  ss << prefix << indent << "Domain:\n";
  if (domainExpr) {
    ss << domainExpr->toStringTree(prefix + indent + indent);
  }
  ss << prefix << indent << "Body:\n";
  if (body) {
    ss << body->toStringTree(prefix + indent + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::statements
