#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class BoolAst final : public ExpressionAst {
  bool value;

public:
  explicit BoolAst(antlr4::Token *token) : ExpressionAst(token), value() {}

  void setValue(const bool value_) { value = value_; }
  bool getValue() const { return value; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions