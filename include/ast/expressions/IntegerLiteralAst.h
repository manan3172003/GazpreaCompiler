#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class IntegerLiteralAst final : public ExpressionAst {
public:
  int integerValue;
  IntegerLiteralAst(antlr4::Token *token, const int integerValue)
      : Ast(token), ExpressionAst(token), integerValue(integerValue) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override { return false; }
};
} // namespace gazprea::ast::expressions