#include "symTable/ArrayTypeSymbol.h"
#include "symTable/StructTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/TypealiasSymbol.h"
#include <sstream>
#include <symTable/Scope.h>

namespace gazprea::symTable {
Scope::~Scope() = default;

std::shared_ptr<Symbol> BaseScope::getSymbol(const std::string &name) {
  if (symbols.find(name) == symbols.end()) {
    return nullptr;
  }
  return symbols[name];
}
std::shared_ptr<Symbol> BaseScope::getTypeSymbol(const std::string &name) {
  if (typeSymbols.find(name) == typeSymbols.end()) {
    return nullptr;
  }
  return typeSymbols[name];
}
void BaseScope::setEnclosingScope(std::shared_ptr<Scope> scope) { enclosingScope = scope; }
std::shared_ptr<Scope> BaseScope::getEnclosingScope() { return enclosingScope.lock(); }
void BaseScope::defineSymbol(std::shared_ptr<Symbol> sym) {
  if (std::dynamic_pointer_cast<Type>(sym) &&
      not std::dynamic_pointer_cast<StructTypeSymbol>(sym)) {
    throw std::runtime_error("Cannot define type symbols here");
  }
  if (const auto structSym = std::dynamic_pointer_cast<StructTypeSymbol>(sym))
    symbols.emplace(structSym->getStructName(), sym);
  else
    symbols.emplace(sym->getName(), sym);
  sym->setScope(shared_from_this());
}

void BaseScope::defineTypeSymbol(std::shared_ptr<Symbol> sym) {
  if (!std::dynamic_pointer_cast<Type>(sym)) {
    throw std::runtime_error("Can only define type symbols");
  }

  if (const auto structSym = std::dynamic_pointer_cast<StructTypeSymbol>(sym))
    typeSymbols.emplace(structSym->getStructName(), sym);
  else
    typeSymbols.emplace(sym->getName(), sym);
  sym->setScope(shared_from_this());
}

std::shared_ptr<Symbol> BaseScope::resolveType(const std::string &type) {
  if (const auto it = typeSymbols.find(type); it != typeSymbols.end()) {
    return it->second;
  }
  if (const auto parent = getEnclosingScope()) {
    return parent->resolveType(type);
  }
  return nullptr;
}
std::shared_ptr<Symbol> GlobalScope::resolveType(const std::string &type) {
  if (type == "integer" || type == "real" || type == "character" || type == "boolean") {
    return getTypeSymbols().at(type);
  }

  if (getTypeSymbols().find(type) == getTypeSymbols().end()) {
    return nullptr;
  }

  // TODO: Error handling check if the symbol exists
  auto typeSym = std::dynamic_pointer_cast<TypealiasSymbol>(getTypeSymbols().at(type));

  // TODO: do the same thing for structs
  if (typeSym->getType()->getName() == "tuple") {
    auto tupleTypeSym = std::dynamic_pointer_cast<TupleTypeSymbol>(typeSym->getType());
    return tupleTypeSym;
  }

  if (typeSym->getType()->getName().substr(0, 5) == "array") {
    return std::dynamic_pointer_cast<ArrayTypeSymbol>(typeSym->getType());
  }

  return resolveType(typeSym->getType()->getName());
}
std::shared_ptr<Symbol> BaseScope::resolveSymbol(const std::string &name) {
  if (const auto it = symbols.find(name); it != symbols.end()) {
    return it->second;
  }
  if (const auto parent = getEnclosingScope()) {
    return parent->resolveSymbol(name);
  }
  return nullptr;
}
std::string Scope::scTypeToString() const {
  switch (scType) {
  case ScopeType::Global:
    return "Global";
  case ScopeType::Local:
    return "Local";
  case ScopeType::Function:
    return "Function";
  case ScopeType::Procedure:
    return "Procedure";
  case ScopeType::Loop:
    return "Loop";
  case ScopeType::Conditional:
    return "Conditional";

  default:
    return "Unknown";
  }
}
std::string BaseScope::toString() {
  std::stringstream ss;
  ss << scTypeToString() << " {";
  for (const auto &[name, symbol] : symbols) {
    ss << symbol->toString() << ", ";
  }
  ss << "}";
  return ss.str();
}
std::string BaseScope::getScopeName() { return "BaseScope"; }
std::string GlobalScope::getScopeName() { return "GlobalScope"; }
std::string LocalScope::getScopeName() { return "LocalScope"; }
} // namespace gazprea::symTable