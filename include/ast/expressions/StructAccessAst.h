#pragma once
#include "ExpressionAst.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::ast::expressions {

class StructAccessAst final : public ExpressionAst {
private:
  std::string structName;
  std::string elementName;

public:
  explicit StructAccessAst(antlr4::Token *token) : ExpressionAst(token) {}

  void setStructName(std::string name) { structName = std::move(name); }
  const std::string &getStructName() const { return structName; }

  void setElementName(const std::string &name) { elementName = name; }
  std::string getElementName() const { return elementName; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override {
    const auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(sym);
    if (variableSymbol && variableSymbol->getQualifier() == Qualifier::Const)
      return false;
    if (variableSymbol && variableSymbol->getQualifier() == Qualifier::Var)
      return true;
    return false;
  }
};

} // namespace gazprea::ast::expressions
