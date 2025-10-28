#pragma once
#include "Type.h"
#include "symTable/Scope.h"
#include <memory>
#include <string>

namespace gazprea::ast {
class Ast;
}

namespace gazprea::symTable {
// Forward declaration
class Scope;

class Symbol {
  std::string name;
  std::weak_ptr<Scope> scope;
  std::weak_ptr<ast::Ast> def;

public:
  explicit Symbol(const std::string &name);

  std::weak_ptr<Scope> getScope();
  std::shared_ptr<ast::Ast> getDef() { return def.lock(); }
  void setScope(std::weak_ptr<Scope> scope_) { scope = scope_; }
  void setDef(std::weak_ptr<ast::Ast> def_) { def = def_; }

  virtual std::string getName();

  std::string scopeToString() const;
  virtual std::string toString();
  virtual ~Symbol() = default;
};
} // namespace gazprea::symTable