#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class CharLiteralAst final : public ExpressionAst {
  std::string value;

public:
  explicit CharLiteralAst(antlr4::Token *token) : ExpressionAst(token) {}

  void setValue(const std::string &value_) { value = value_; }
  std::string getValue() const { return value; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions