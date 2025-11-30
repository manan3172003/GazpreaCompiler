#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class BoolLiteralAst final : public ExpressionAst {
  bool value;

public:
  explicit BoolLiteralAst(antlr4::Token *token) : Ast(token), ExpressionAst(token), value() {}

  void setValue(const bool value_) { value = value_; }
  bool getValue() const { return value; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;

  bool isLValue() override { return false; }
};
} // namespace gazprea::ast::expressions