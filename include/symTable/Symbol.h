#pragma once
#include "Type.h"
#include "symTable/Scope.h"
#include <memory>
#include <string>

namespace gazprea::symTable {
// Forward declaration
class Scope;

class Symbol {
  std::string name;
  std::weak_ptr<Scope> scope;

public:
  explicit Symbol(const std::string &name);
  Symbol(std::string name, std::shared_ptr<symTable::Type> type);
  Symbol(std::string name, std::shared_ptr<symTable::Type> type,
         std::shared_ptr<Scope> scope);

  std::weak_ptr<Scope> getScope();
  void setScope(std::weak_ptr<Scope> scope_) { scope = std::move(scope_); }

  virtual std::string getName();
  virtual std::string toString();
  virtual ~Symbol() = default;
};
} // namespace gazprea::symTable