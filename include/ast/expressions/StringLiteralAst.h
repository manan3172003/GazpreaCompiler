#pragma once
#include "ast/expressions/ExpressionAst.h"

namespace gazprea::ast::expressions {
class StringLiteralAst final : public ExpressionAst {
  std::string value;

public:
  explicit StringLiteralAst(antlr4::Token *token) : ExpressionAst(token) {}

  void setValue(const std::string &value_) { value = value_; }
  std::string getValue() const { return value; }

  bool isLValue() override { return false; }
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions