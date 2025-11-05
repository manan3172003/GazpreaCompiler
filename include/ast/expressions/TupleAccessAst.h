#pragma once
#include "ExpressionAst.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::ast::expressions {

class TupleAccessAst final : public ExpressionAst {
private:
  std::string tupleName;
  int32_t fieldIndex;

public:
  explicit TupleAccessAst(antlr4::Token *token) : ExpressionAst(token), tupleName(), fieldIndex() {}

  void setTupleName(std::string name) { tupleName = std::move(name); }
  const std::string &getTupleName() const { return tupleName; }

  void setFieldIndex(const int32_t index) { fieldIndex = index; }
  int32_t getFieldIndex() const { return fieldIndex; }

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
