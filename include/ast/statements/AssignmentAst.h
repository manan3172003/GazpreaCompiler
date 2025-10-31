#pragma once
#include "AssignLeftAst.h"
#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"

namespace gazprea::ast::statements {
class AssignmentAst : public StatementAst {
  std::shared_ptr<AssignLeftAst> lVal;
  std::shared_ptr<expressions::ExpressionAst> expr;

public:
  explicit AssignmentAst(antlr4::Token *token) : StatementAst(token) {}
  std::shared_ptr<AssignLeftAst> getLVal() const { return lVal; }
  std::shared_ptr<expressions::ExpressionAst> getExpr() const { return expr; }
  void setLVal(std::shared_ptr<AssignLeftAst> left) { lVal = left; }
  void setExpr(std::shared_ptr<expressions::ExpressionAst> expression) { expr = expression; }
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~AssignmentAst() override = default;
};
} // namespace gazprea::ast::statements