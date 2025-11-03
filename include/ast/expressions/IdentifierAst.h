#pragma once
#include "ExpressionAst.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::ast::expressions {
class IdentifierAst final : public ExpressionAst {
  std::string name;

public:
  explicit IdentifierAst(antlr4::Token *token) : ExpressionAst(token) {}
  void setName(const std::string &name_) { name = name_; }
  std::string getName() const { return name; }
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