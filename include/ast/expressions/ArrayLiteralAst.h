#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {

class ArrayLiteralAst : public ExpressionAst {
  std::vector<std::shared_ptr<ExpressionAst>> elements;

public:
  explicit ArrayLiteralAst(antlr4::Token *token) : ExpressionAst(token) {}
  void addElement(std::shared_ptr<ExpressionAst> element);
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override { return false; };
};
} // namespace gazprea::ast::expressions