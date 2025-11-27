#pragma once
#include "DataTypeAst.h"
#include "ast/expressions/ExpressionAst.h"

#include <vector>

namespace gazprea::ast::types {

class ArrayTypeAst final : public DataTypeAst {
  std::shared_ptr<DataTypeAst> type;
  std::vector<std::shared_ptr<expressions::ExpressionAst>> static_sizes = {};

public:
  explicit ArrayTypeAst(antlr4::Token *token) : DataTypeAst(token) {}
  void setType(const std::shared_ptr<DataTypeAst> &_type) { type = _type; };
  std::shared_ptr<DataTypeAst> getType() { return type; }
  void pushSize(const std::shared_ptr<expressions::ExpressionAst> &sizeAst) {
    static_sizes.push_back(sizeAst);
  }
  std::vector<std::shared_ptr<expressions::ExpressionAst>> getSizes() { return static_sizes; }
  std::vector<bool> isSizeInferred() const;
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::types