#pragma once
#include "Symbol.h"

#include <memory>
#include <mlir/IR/Value.h>
#include <string>
#include <unordered_map>

namespace gazprea::symTable {
// Forward declaration
class Symbol;

enum class ScopeType { Global, Local, Function, Procedure, Loop, Conditional };

class Scope : public std::enable_shared_from_this<Scope> {
  std::vector<std::pair<std::shared_ptr<Type>, mlir::Value>> scopeStack;

protected:
  ScopeType scType;

public:
  explicit Scope(const ScopeType scType) : scType(scType) {}
  virtual std::string getScopeName() = 0;
  virtual void setEnclosingScope(std::shared_ptr<Scope> scope) = 0;
  virtual std::shared_ptr<Scope> getEnclosingScope() = 0;
  virtual void defineSymbol(std::shared_ptr<Symbol> sym) = 0;
  virtual void defineTypeSymbol(std::shared_ptr<Symbol> sym) = 0;
  virtual std::shared_ptr<Symbol> resolveType(const std::string &name) = 0;
  virtual std::shared_ptr<Symbol> resolveSymbol(const std::string &name) = 0;
  virtual std::shared_ptr<Symbol> getSymbol(const std::string &name) = 0;
  virtual std::shared_ptr<Symbol> getTypeSymbol(const std::string &name) = 0;
  virtual std::string toString() = 0;
  void pushElementToScopeStack(std::shared_ptr<Type> elementType, mlir::Value val) {
    scopeStack.push_back(std::make_pair(elementType, val));
  }
  void popElementFromScopeStack() { scopeStack.pop_back(); }
  std::pair<std::shared_ptr<Type>, mlir::Value> getTopElementInStack() { return scopeStack.back(); }
  std::vector<std::pair<std::shared_ptr<Type>, mlir::Value>> &getScopeStack() { return scopeStack; }
  ScopeType getScopeType() const { return scType; }
  std::string scTypeToString() const;
  virtual ~Scope();
};

class BaseScope : public Scope {
  std::weak_ptr<Scope> enclosingScope;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> typeSymbols;

public:
  explicit BaseScope(const ScopeType scType) : Scope(scType) {};
  std::string getScopeName() override;
  void setEnclosingScope(std::shared_ptr<Scope> scope) override;
  std::shared_ptr<Scope> getEnclosingScope() override;
  void defineSymbol(std::shared_ptr<Symbol> sym) override;
  void defineTypeSymbol(std::shared_ptr<Symbol> sym) override;
  std::shared_ptr<Symbol> resolveType(const std::string &name) override;
  std::shared_ptr<Symbol> resolveSymbol(const std::string &name) override;
  std::string toString() override;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> &getSymbols() { return symbols; }
  std::unordered_map<std::string, std::shared_ptr<Symbol>> &getTypeSymbols() { return typeSymbols; }
  std::shared_ptr<Symbol> getSymbol(const std::string &name) override;
  std::shared_ptr<Symbol> getTypeSymbol(const std::string &name) override;
};

class GlobalScope final : public BaseScope {
public:
  GlobalScope() : BaseScope(ScopeType::Global) {};
  std::string getScopeName() override;
  std::shared_ptr<Symbol> resolveType(const std::string &name) override;
};

class LocalScope final : public BaseScope {
public:
  LocalScope() : BaseScope(ScopeType::Local) {};
  std::string getScopeName() override;
  void setScopeName(ScopeType scopeType) { scType = scopeType; }
};
} // namespace gazprea::symTable