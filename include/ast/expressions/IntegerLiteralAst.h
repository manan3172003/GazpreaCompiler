#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class IntegerLiteralAst final : public ExpressionAst {
public:
  int integerValue;
  IntegerLiteralAst(antlr4::Token *token, const int integerValue)
      : ExpressionAst(token), integerValue(integerValue) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions