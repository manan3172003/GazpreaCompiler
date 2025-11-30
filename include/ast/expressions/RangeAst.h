#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {

class RangeAst final : public ExpressionAst {
private:
  std::shared_ptr<ExpressionAst> start;
  std::shared_ptr<ExpressionAst> end;

public:
  explicit RangeAst(antlr4::Token *token) : Ast(token), ExpressionAst(token) {}

  void setStart(std::shared_ptr<ExpressionAst> startExpr) { start = startExpr; }
  void setEnd(std::shared_ptr<ExpressionAst> endExpr) { end = endExpr; }

  std::shared_ptr<ExpressionAst> getStart() const { return start; }
  std::shared_ptr<ExpressionAst> getEnd() const { return end; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override { return false; }
};

} // namespace gazprea::ast::expressions