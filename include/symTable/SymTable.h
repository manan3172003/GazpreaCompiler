#pragma once
#include "Scope.h"

#include <memory>

namespace gazprea::symTable {
class SymbolTable {
protected:
  void initTypeSystem() const;

public:
  SymbolTable();

  void pushScope(std::shared_ptr<Scope> child);
  void popScope();

  std::string toString() const;
};
} // namespace gazprea::symTable