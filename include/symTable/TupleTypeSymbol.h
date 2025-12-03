#pragma once
#include "Symbol.h"
#include "ast/types/DataTypeAst.h"
#include <vector>

namespace gazprea::symTable {
class TupleTypeSymbol final : public Type, public Symbol {
  std::vector<std::shared_ptr<Type>> resolvedTypes;
  std::vector<std::shared_ptr<ast::types::DataTypeAst>> unresolvedTypes;

public:
  explicit TupleTypeSymbol(const std::string &name) : Symbol(name) {};
  void addResolvedType(std::shared_ptr<Type> type) { resolvedTypes.push_back(type); }
  void addUnresolvedType(std::shared_ptr<ast::types::DataTypeAst> type) {
    unresolvedTypes.push_back(type);
  }

  const std::shared_ptr<Type> &getResolvedType(const size_t index) {
    return resolvedTypes[index - 1];
  }
  const std::shared_ptr<ast::types::DataTypeAst> &getUnresolvedType(const size_t index) {
    return unresolvedTypes[index - 1];
  }
  const std::vector<std::shared_ptr<Type>> &getResolvedTypes() const { return resolvedTypes; }
  const std::vector<std::shared_ptr<ast::types::DataTypeAst>> &getUnresolvedTypes() const {
    return unresolvedTypes;
  }
  std::string getName() override;
  std::string toString() override;
};
} // namespace gazprea::symTable