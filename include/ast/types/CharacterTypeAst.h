#pragma once
#include "DataTypeAst.h"

namespace gazprea::ast::types {

class CharacterTypeAst final : public DataTypeAst {
public:
  explicit CharacterTypeAst(antlr4::Token *token) : DataTypeAst(token) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::types