#pragma once
#include "Scope.h"
#include "Symbol.h"

namespace gazprea::symTable {
class ScopedSymbol : public Symbol, public BaseScope {
public:
  ScopedSymbol(const std::string &name, const ScopeType scType)
      : Symbol(name), BaseScope(scType) {};
};
} // namespace gazprea::symTable