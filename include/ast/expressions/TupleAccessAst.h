#pragma once
#include "ExpressionAst.h"

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
};

} // namespace gazprea::ast::expressions
