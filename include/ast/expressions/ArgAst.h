#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class ArgAst final : public ExpressionAst {
  std::shared_ptr<ExpressionAst> expr;

public:
  explicit ArgAst(antlr4::Token *token) : Ast(token), ExpressionAst(token) {}

  void setExpr(std::shared_ptr<ExpressionAst> expr_) { expr = expr_; }
  std::shared_ptr<ExpressionAst> getExpr() const { return expr; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;

  bool isLValue() override { return expr->isLValue(); }
};
} // namespace gazprea::ast::expressions