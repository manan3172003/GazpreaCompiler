#pragma once
#include "ScopedSymbol.h"

namespace gazprea::symTable {
class MethodSymbol final : public ScopedSymbol {
public:
  MethodSymbol(const std::string &name, const ScopeType scType)
      : ScopedSymbol(name, scType) {}
};
} // namespace gazprea::symTable