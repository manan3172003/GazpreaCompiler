#pragma once
#include "Symbol.h"
#include <vector>

namespace gazprea::symTable {
class TupleTypeSymbol final : public Type, public Symbol {
  std::vector<std::shared_ptr<Type>> resolvedTypes;
  std::vector<std::string> unresolvedTypes;

public:
  explicit TupleTypeSymbol(const std::string &name) : Symbol(name) {};
  void addResolvedType(std::shared_ptr<Type> type) { resolvedTypes.push_back(type); }
  const std::vector<std::shared_ptr<Type>> &getResolvedTypes() const { return resolvedTypes; }
  void addUnresolvedType(const std::string &type) { unresolvedTypes.push_back(type); }
  const std::vector<std::string> &getUnresolvedTypes() const { return unresolvedTypes; }
  std::string getName() override;
  std::string toString() override;
};
} // namespace gazprea::symTable