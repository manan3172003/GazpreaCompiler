#pragma once
#include "Scope.h"

#include <memory>

namespace gazprea::symTable {
class SymbolTable {
  std::shared_ptr<GlobalScope> globalScope;
  std::shared_ptr<Scope> currentScope;

protected:
  void initTypeSystem() const;

public:
  SymbolTable();

  void pushScope(std::shared_ptr<Scope> child);
  void popScope();

  std::shared_ptr<GlobalScope> getGlobalScope();
  void setGlobalScope(std::shared_ptr<GlobalScope> scope);
  std::shared_ptr<Scope> getCurrentScope();
  void setCurrentScope(std::shared_ptr<Scope> scope);

  std::string toString() const;
};
} // namespace gazprea::symTable