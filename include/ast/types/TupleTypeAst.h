#pragma once
#include "DataTypeAst.h"

namespace gazprea::ast::types {

class TupleTypeAst final : public DataTypeAst {
private:
  std::vector<std::shared_ptr<DataTypeAst>> types;

public:
  explicit TupleTypeAst(antlr4::Token *token) : DataTypeAst(token) {}
  void addType(std::shared_ptr<DataTypeAst> type) { types.push_back(type); }
  std::vector<std::shared_ptr<DataTypeAst>> getTypes() { return types; }
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::types