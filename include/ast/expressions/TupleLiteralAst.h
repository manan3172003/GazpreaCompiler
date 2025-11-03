#pragma once
#include "ExpressionAst.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::ast::expressions {
class TupleLiteralAst : public ExpressionAst {
private:
  std::vector<std::shared_ptr<ExpressionAst>> elements;

public:
  explicit TupleLiteralAst(antlr4::Token *token) : ExpressionAst(token) {}

  void addElement(std::shared_ptr<ExpressionAst> element);
  std::vector<std::shared_ptr<ExpressionAst>> getElements() const { return elements; }
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override {
    const auto variableDeclaration =
        std::dynamic_pointer_cast<symTable::VariableSymbol>(sym->getDef());
    if (variableDeclaration && variableDeclaration->getQualifier() == Qualifier::Const)
      return false;
    if (variableDeclaration && variableDeclaration->getQualifier() == Qualifier::Var)
      return true;
    return false;
  }
};
} // namespace gazprea::ast::expressions