#pragma once
#include "Symbol.h"

#include <map>
#include <memory>
#include <string>

namespace gazprea::symTable {
// Forward declaration
class Symbol;

enum class ScopeType { Global, Local, Function, Procedure };

class Scope : public std::enable_shared_from_this<Scope> {
public:
  virtual std::string getScopeName() = 0;
  virtual ScopeType getScopeType() = 0;
  virtual void setEnclosingScope(std::shared_ptr<Scope> scope) = 0;
  virtual std::shared_ptr<Scope> getEnclosingScope() = 0;
  virtual void define(std::shared_ptr<Symbol> sym) = 0;
  virtual std::shared_ptr<Symbol> resolve(const std::string &name) = 0;
  virtual std::string toString() = 0;
  virtual ~Scope();
};

class BaseScope : public Scope {
  std::weak_ptr<Scope> enclosingScope;
  std::map<std::string, std::shared_ptr<Symbol>> symbols;
  ScopeType scType;

public:
  explicit BaseScope(const ScopeType scType) : scType(scType) {};
  std::string getScopeName() override;
  ScopeType getScopeType() override;
  void setEnclosingScope(std::shared_ptr<Scope> scope) override;
  std::shared_ptr<Scope> getEnclosingScope() override;
  void define(std::shared_ptr<Symbol> sym) override;
  std::shared_ptr<Symbol> resolve(const std::string &name) override;
  std::string scTypeToString() const;
  std::string toString() override;
};

class GlobalScope final : public BaseScope {
public:
  GlobalScope() : BaseScope(ScopeType::Global) {};
  std::string getScopeName() override;
};

class LocalScope final : public BaseScope {
  std::string name;

public:
  LocalScope() : BaseScope(ScopeType::Local) {};
  std::string getScopeName() override;
};
} // namespace gazprea::symTable