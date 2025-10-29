#pragma once
#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"

namespace gazprea::ast::statements {
class ReturnAst final : public StatementAst {
  std::shared_ptr<expressions::ExpressionAst> expr;

public:
  explicit ReturnAst(antlr4::Token *token)
      : StatementAst(token), expr(nullptr) {}
  void setExpr(std::shared_ptr<expressions::ExpressionAst> expression) {
    expr = expression;
  }
  std::shared_ptr<expressions::ExpressionAst> getExpr() const { return expr; }
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~ReturnAst() override = default;
};
} // namespace gazprea::ast::statements