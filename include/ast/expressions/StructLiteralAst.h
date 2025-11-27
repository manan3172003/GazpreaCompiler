#pragma once
#include "ExpressionAst.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::ast::expressions {
class StructLiteralAst final : public ExpressionAst {
private:
  std::string structTypeName;
  std::vector<std::shared_ptr<ExpressionAst>> elements;

public:
  explicit StructLiteralAst(antlr4::Token *token) : ExpressionAst(token) {}

  void addElement(const std::shared_ptr<ExpressionAst> &element);
  void setStructTypeName(const std::string &name);

  std::string getStructTypeName() const;
  std::vector<std::shared_ptr<ExpressionAst>> getElements() const;

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override;
};
} // namespace gazprea::ast::expressions