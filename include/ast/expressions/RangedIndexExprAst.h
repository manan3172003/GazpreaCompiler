#pragma once
#include "ExpressionAst.h"
#include "IndexExprAst.h"

namespace gazprea::ast::expressions {

class RangedIndexExprAst final : public IndexExprAst {
private:
  std::shared_ptr<ExpressionAst> leftIndexExpr;
  std::shared_ptr<ExpressionAst> rightIndexExpr;

public:
  explicit RangedIndexExprAst(antlr4::Token *token) : IndexExprAst(token) {}

  std::shared_ptr<ExpressionAst> getLeftIndexExpr() const { return leftIndexExpr; }
  std::shared_ptr<ExpressionAst> getRightIndexExpr() const { return rightIndexExpr; }
  void setLeftIndexExpr(const std::shared_ptr<ExpressionAst> &expression) {
    leftIndexExpr = expression;
  }
  void setRightIndexExpr(const std::shared_ptr<ExpressionAst> &expression) {
    rightIndexExpr = expression;
  }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::expressions