#pragma once
#include "BlockAst.h"
#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"
#include <memory>

namespace gazprea::ast::statements {

class IteratorLoopAst : public StatementAst {
private:
  std::shared_ptr<BlockAst> body;
  std::string iteratorName;
  std::shared_ptr<expressions::ExpressionAst> domainExpr;

public:
  explicit IteratorLoopAst(antlr4::Token *token) : Ast(token), StatementAst(token) {}
  void setBody(std::shared_ptr<BlockAst> bodyBlock);
  void setIteratorName(std::string name);
  void setDomainExpr(std::shared_ptr<expressions::ExpressionAst> expr);
  std::shared_ptr<BlockAst> getBody() const;
  std::string getIteratorName() const;
  std::shared_ptr<expressions::ExpressionAst> getDomainExpr() const;
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~IteratorLoopAst() override = default;
};

} // namespace gazprea::ast::statements
