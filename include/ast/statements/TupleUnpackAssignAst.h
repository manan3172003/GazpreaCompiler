#pragma once
#include "AssignLeftAst.h"

namespace gazprea::ast::statements {

class TupleUnpackAssignAst final : public AssignLeftAst {
  std::vector<std::shared_ptr<AssignLeftAst>> lVals;

public:
  explicit TupleUnpackAssignAst(antlr4::Token *token) : AssignLeftAst(token) {}

  const std::vector<std::shared_ptr<AssignLeftAst>> &getLVals() const { return lVals; }
  void addSubLVal(std::shared_ptr<AssignLeftAst> lVal);

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::statements