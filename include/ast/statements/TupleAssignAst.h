#pragma once
#include "AssignmentAst.h"

namespace gazprea::ast::statements {

class TupleAssignAst : public AssignLeftAst {
private:
  std::string tupleName;
  int32_t fieldIndex;

public:
  explicit TupleAssignAst(antlr4::Token *token)
      : AssignLeftAst(token), tupleName(), fieldIndex() {}

  void setTupleName(std::string name) { tupleName = std::move(name); }
  const std::string &getTupleName() const { return tupleName; }

  void setFieldIndex(const int32_t index) { fieldIndex = index; }
  int32_t getFieldIndex() const { return fieldIndex; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::statements