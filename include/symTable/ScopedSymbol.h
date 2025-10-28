#pragma once
#include "Scope.h"
#include "Symbol.h"
#include <vector>

namespace gazprea::symTable {
class ScopedSymbol : public Symbol, public Scope {
  std::weak_ptr<Scope> enclosingScope;
  std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> symbols;

public:
  ScopedSymbol(const std::string &name, const ScopeType scType)
      : Symbol(name), Scope(scType) {};
  std::string getScopeName() override;
  void setEnclosingScope(std::shared_ptr<Scope> scope) override;
  std::shared_ptr<Scope> getEnclosingScope() override;
  void define(std::shared_ptr<Symbol> sym) override;
  std::shared_ptr<Symbol> resolve(const std::string &name) override;
  std::string toString() override;
  const std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> &
  getSymbols() const {
    return symbols;
  }
};
} // namespace gazprea::symTable