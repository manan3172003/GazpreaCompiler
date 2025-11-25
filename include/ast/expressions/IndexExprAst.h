#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {

class IndexExprAst : public ExpressionAst {
public:
  explicit IndexExprAst(antlr4::Token *token) : ExpressionAst(token) {}
  NodeType getNodeType() const override { return NodeType::IndexExpr; }
  bool isLValue() override { return false; }
};

} // namespace gazprea::ast::expressions