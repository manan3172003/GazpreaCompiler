#pragma once
#include "ScopedSymbol.h"

namespace gazprea::symTable {
class MethodSymbol final : public ScopedSymbol {
  std::shared_ptr<Type> returnType;

public:
  MethodSymbol(const std::string &name, const ScopeType scType)
      : ScopedSymbol(name, scType) {}
  void setReturnType(std::shared_ptr<Type> type) { returnType = type; }
  std::shared_ptr<Type> getReturnType() const { return returnType; }
  std::string toString() override;
};
} // namespace gazprea::symTable