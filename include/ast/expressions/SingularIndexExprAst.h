#pragma once
#include "ExpressionAst.h"
#include "IndexExprAst.h"

namespace gazprea::ast::expressions {

class SingularIndexExprAst final : public IndexExprAst {
private:
  std::shared_ptr<ExpressionAst> singularIndexExpr;

public:
  explicit SingularIndexExprAst(antlr4::Token *token) : Ast(token), IndexExprAst(token) {}

  std::shared_ptr<ExpressionAst> getSingularIndexExpr() const { return singularIndexExpr; }
  void setSingularIndexExpr(const std::shared_ptr<ExpressionAst> &expression) {
    singularIndexExpr = expression;
  }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::expressions