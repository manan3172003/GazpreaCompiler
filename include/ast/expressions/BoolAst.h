#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class BoolAst final : public ExpressionAst {
  BoolValue value;

public:
  explicit BoolAst(antlr4::Token *token) : ExpressionAst(token), value() {}

  void setValue(const BoolValue value_) { value = value_; }
  BoolValue getValue() const { return value; }

  std::string boolToString() const;
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions