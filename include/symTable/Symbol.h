#pragma once
#include "symTable/Scope.h"
#include "types/Type.h"
#include <memory>
#include <string>

namespace gazprea::symTable {
// Forward declaration
class Scope;

class Symbol {
public:
  explicit Symbol(const std::string &name);
  Symbol(std::string name, std::shared_ptr<types::Type> type);
  Symbol(std::string name, std::shared_ptr<types::Type> type,
         std::shared_ptr<Scope> scope);
  virtual std::string getName();

  virtual std::string toString();
  virtual ~Symbol() = default;
};
} // namespace gazprea::symTable