#pragma once
#include "DataTypeAst.h"

namespace gazprea::ast::types {
class VectorTypeAst final : public DataTypeAst {
  std::shared_ptr<DataTypeAst> elementType;

public:
  explicit VectorTypeAst(antlr4::Token *token) : DataTypeAst(token) {}

  std::shared_ptr<DataTypeAst> getElementType() { return elementType; }
  void setElementType(const std::shared_ptr<DataTypeAst> &type) { elementType = type; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::types