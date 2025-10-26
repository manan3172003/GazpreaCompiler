#pragma once
#include "DataTypeAst.h"

namespace gazprea::ast::types {

class AliasTypeAst final : public DataTypeAst {
  std::string alias;

public:
  explicit AliasTypeAst(antlr4::Token *token) : DataTypeAst(token) {}
  NodeType getNodeType() const override;
  std::string getAlias() const { return alias; }
  void setAlias(std::string alias_) { this->alias = std::move(alias_); }
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::types