#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class IdentifierAst final : public ExpressionAst {
  std::string name;

public:
  explicit IdentifierAst(antlr4::Token *token) : ExpressionAst(token) {}
  void setName(const std::string &name_) { name = name_; }
  std::string getName() const { return name; }
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions