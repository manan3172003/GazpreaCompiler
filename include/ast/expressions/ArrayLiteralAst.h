#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {

class ArrayLiteralAst final : public ExpressionAst {
  std::vector<std::shared_ptr<ExpressionAst>> elements;

public:
  explicit ArrayLiteralAst(antlr4::Token *token) : Ast(token), ExpressionAst(token) {}
  void addElement(std::shared_ptr<ExpressionAst> element);
  std::vector<std::shared_ptr<ExpressionAst>> getElements() const { return elements; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override { return false; };
};
} // namespace gazprea::ast::expressions