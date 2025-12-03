#pragma once
#include "AssignmentAst.h"
#include "ast/expressions/IndexExprAst.h"

namespace gazprea::ast::statements {

class ArrayElementAssignAst final : public AssignLeftAst {
private:
  std::shared_ptr<AssignLeftAst> arrayInstance;
  std::shared_ptr<expressions::IndexExprAst> elementIndex;

public:
  explicit ArrayElementAssignAst(antlr4::Token *token) : AssignLeftAst(token) {}

  void setArrayInstance(const std::shared_ptr<AssignLeftAst> &instance) {
    arrayInstance = instance;
  }
  std::shared_ptr<AssignLeftAst> getArrayInstance() const { return arrayInstance; }

  void setElementIndex(const std::shared_ptr<expressions::IndexExprAst> &index) {
    elementIndex = index;
  }
  std::shared_ptr<expressions::IndexExprAst> getElementIndex() const { return elementIndex; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::statements