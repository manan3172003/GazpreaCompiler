#pragma once
#include "ExpressionAst.h"
#include <string>

namespace gazprea::ast::expressions {

class DomainExprAst final : public ExpressionAst {
  std::string iteratorName;
  std::shared_ptr<ExpressionAst> domainExpression;

public:
  explicit DomainExprAst(antlr4::Token *token) : Ast(token), ExpressionAst(token) {}

  void setIteratorName(const std::string &name) { iteratorName = name; }
  std::string getIteratorName() const { return iteratorName; }

  void setDomainExpression(std::shared_ptr<ExpressionAst> expr) { domainExpression = expr; }
  std::shared_ptr<ExpressionAst> getDomainExpression() const { return domainExpression; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override { return false; }
};

} // namespace gazprea::ast::expressions
