#pragma once
#include "BlockAst.h"
#include "StatementAst.h"
#include "ast/expressions/DomainExprAst.h"
#include "ast/expressions/ExpressionAst.h"
#include <memory>

namespace gazprea::ast::statements {

class IteratorLoopAst : public StatementAst {
private:
  std::shared_ptr<BlockAst> body;
  std::shared_ptr<expressions::DomainExprAst> domain;

public:
  explicit IteratorLoopAst(antlr4::Token *token) : Ast(token), StatementAst(token) {}
  void setBody(std::shared_ptr<BlockAst> bodyBlock);
  void setDomain(std::shared_ptr<expressions::DomainExprAst> domain);
  std::shared_ptr<BlockAst> getBody() const;
  std::shared_ptr<expressions::DomainExprAst> getDomain() const;
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~IteratorLoopAst() override = default;
};

} // namespace gazprea::ast::statements
