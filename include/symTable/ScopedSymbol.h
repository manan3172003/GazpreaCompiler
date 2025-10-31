#pragma once
#include "Scope.h"
#include "Symbol.h"
#include <vector>

namespace gazprea::symTable {
class ScopedSymbol : public Symbol, public Scope {
  std::weak_ptr<Scope> enclosingScope;
  std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> symbols;
  // For structs
  std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> typeSymbols;

public:
  ScopedSymbol(const std::string &name, const ScopeType scType) : Symbol(name), Scope(scType) {};
  std::string getScopeName() override;
  void setEnclosingScope(std::shared_ptr<Scope> scope) override;
  std::shared_ptr<Scope> getEnclosingScope() override;
  void defineSymbol(std::shared_ptr<Symbol> sym) override;
  void defineTypeSymbol(std::shared_ptr<Symbol> sym) override;
  std::shared_ptr<Symbol> resolveType(const std::string &name) override;
  std::shared_ptr<Symbol> resolveSymbol(const std::string &name) override;
  std::string toString() override;
  const std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> &getSymbols() const {
    return symbols;
  }
  const std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> &getTypeSymbols() const {
    return symbols;
  }
  std::shared_ptr<Symbol> getSymbol(const std::string &name) override;
  std::shared_ptr<Symbol> getTypeSymbol(const std::string &name) override;
};
} // namespace gazprea::symTable