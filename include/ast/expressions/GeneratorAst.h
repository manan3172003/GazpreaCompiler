#pragma once
#include "DomainExprAst.h"
#include "ExpressionAst.h"
#include <vector>

namespace gazprea::ast::expressions {

class GeneratorAst final : public ExpressionAst {
  std::vector<std::shared_ptr<DomainExprAst>> domainExprs;
  std::shared_ptr<ExpressionAst> generatorExpression;

public:
  explicit GeneratorAst(antlr4::Token *token) : Ast(token), ExpressionAst(token) {}

  void addDomainExpr(std::shared_ptr<DomainExprAst> expr) { domainExprs.push_back(expr); }
  const std::vector<std::shared_ptr<DomainExprAst>> &getDomainExprs() const { return domainExprs; }
  size_t getDimensionCount() const { return domainExprs.size(); }

  void setGeneratorExpression(std::shared_ptr<ExpressionAst> expr) { generatorExpression = expr; }
  std::shared_ptr<ExpressionAst> getGeneratorExpression() const { return generatorExpression; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override { return false; }
};

} // namespace gazprea::ast::expressions