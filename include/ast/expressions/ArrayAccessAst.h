#pragma once
#include "ExpressionAst.h"
#include "IndexExprAst.h"

namespace gazprea::ast::expressions {

class ArrayAccessAst final : public ExpressionAst {
private:
  std::shared_ptr<ExpressionAst> arrayInstance;
  std::shared_ptr<IndexExprAst> elementIndex;

public:
  explicit ArrayAccessAst(antlr4::Token *token) : ExpressionAst(token) {}

  void setArrayInstance(const std::shared_ptr<ExpressionAst> &instance) {
    arrayInstance = instance;
  }
  std::shared_ptr<ExpressionAst> getArrayName() const { return arrayInstance; }

  void setElementIndex(const std::shared_ptr<IndexExprAst> &index) { elementIndex = index; }
  std::shared_ptr<IndexExprAst> getElementIndex() const { return elementIndex; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override { return arrayInstance->isLValue(); }
};

} // namespace gazprea::ast::expressions
