#include "ast/statements/IteratorLoopAst.h"

namespace gazprea::ast::statements {

void IteratorLoopAst::setBody(std::shared_ptr<BlockAst> bodyBlock) { this->body = bodyBlock; }

void IteratorLoopAst::setDomain(std::shared_ptr<expressions::DomainExprAst> domain) {
  this->domain = domain;
}

std::shared_ptr<BlockAst> IteratorLoopAst::getBody() const { return this->body; }

std::shared_ptr<expressions::DomainExprAst> IteratorLoopAst::getDomain() const {
  return this->domain;
}

NodeType IteratorLoopAst::getNodeType() const { return NodeType::IteratorLoop; }

std::string IteratorLoopAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "IteratorLoop: \n";
  ss << prefix << indent << "Domain:\n";
  if (domain) {
    ss << domain->toStringTree(prefix + indent + indent);
  }
  ss << prefix << indent << "Body:\n";
  if (body) {
    ss << body->toStringTree(prefix + indent + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::statements
