#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class CharLiteralAst final : public ExpressionAst {
  char value;

public:
  explicit CharLiteralAst(antlr4::Token *token) : Ast(token), ExpressionAst(token) {}

  void setValue(char value_) { value = value_; }
  char getValue() const { return value; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;

  bool isLValue() override { return false; }
};
} // namespace gazprea::ast::expressions