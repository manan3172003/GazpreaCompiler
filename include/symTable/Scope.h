#pragma once
#include "Symbol.h"

#include <memory>
#include <string>

namespace gazprea::symTable {
// Forward declaration
class Symbol;

class Scope : public std::enable_shared_from_this<Scope> {
public:
  virtual std::string getScopeName() = 0;
  virtual void setEnclosingScope(std::shared_ptr<Scope> scope) = 0;
  virtual std::shared_ptr<Scope> getEnclosingScope() = 0;
  virtual void define(std::shared_ptr<Symbol> sym) = 0;
  virtual std::shared_ptr<Symbol> resolve(const std::string &name) = 0;
  virtual std::string toString() = 0;
  virtual ~Scope();
};
} // namespace gazprea::symTable