#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class IntegerAst final : public ExpressionAst {
public:
  int integerValue;
  IntegerAst(antlr4::Token *token, const int integerValue)
      : ExpressionAst(token), integerValue(integerValue) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions