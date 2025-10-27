#pragma once
#include "Symbol.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace gazprea::symTable {
// Forward declaration
class Symbol;

enum class ScopeType { Global, Local, Function, Procedure };

class Scope : public std::enable_shared_from_this<Scope> {
  ScopeType scType;

public:
  explicit Scope(const ScopeType scType) : scType(scType) {}
  virtual std::string getScopeName() = 0;
  virtual void setEnclosingScope(std::shared_ptr<Scope> scope) = 0;
  virtual std::shared_ptr<Scope> getEnclosingScope() = 0;
  virtual void define(std::shared_ptr<Symbol> sym) = 0;
  virtual std::shared_ptr<Symbol> resolve(const std::string &name) = 0;
  virtual std::string toString() = 0;
  ScopeType getScopeType() const { return scType; }
  std::string scTypeToString() const;
  virtual ~Scope();
};

class BaseScope : public Scope {
  std::weak_ptr<Scope> enclosingScope;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;

public:
  explicit BaseScope(const ScopeType scType) : Scope(scType) {};
  std::string getScopeName() override;
  void setEnclosingScope(std::shared_ptr<Scope> scope) override;
  std::shared_ptr<Scope> getEnclosingScope() override;
  void define(std::shared_ptr<Symbol> sym) override;
  std::shared_ptr<Symbol> resolve(const std::string &name) override;
  std::string toString() override;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> &getSymbols() {
    return symbols;
  }
};

class GlobalScope final : public BaseScope {
public:
  GlobalScope() : BaseScope(ScopeType::Global) {};
  std::string getScopeName() override;
};

class LocalScope final : public BaseScope {
public:
  LocalScope() : BaseScope(ScopeType::Local) {};
  std::string getScopeName() override;
};
} // namespace gazprea::symTable