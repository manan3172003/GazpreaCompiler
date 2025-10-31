#include <sstream>
#include <symTable/ScopedSymbol.h>

namespace gazprea::symTable {
std::string ScopedSymbol::getScopeName() { return "ScopedSymbol"; }
void ScopedSymbol::setEnclosingScope(std::shared_ptr<Scope> scope) { enclosingScope = scope; }
std::shared_ptr<Scope> ScopedSymbol::getEnclosingScope() { return enclosingScope.lock(); }
std::shared_ptr<Symbol> ScopedSymbol::getSymbol(const std::string &name) {
  for (const auto &[_name, symbol] : symbols) {
    if (name == _name)
      return symbol;
  }
  return nullptr;
}
std::shared_ptr<Symbol> ScopedSymbol::getTypeSymbol(const std::string &name) {
  for (const auto &[_name, symbol] : symbols) {
    if (_name == name)
      return symbol;
  }
  return nullptr;
}

void ScopedSymbol::defineSymbol(std::shared_ptr<Symbol> sym) {
  symbols.emplace_back(sym->getName(), sym);
  sym->setScope(shared_from_this());
}
void ScopedSymbol::defineTypeSymbol(std::shared_ptr<Symbol> sym) {
  typeSymbols.emplace_back(sym->getName(), sym);
  sym->setScope(shared_from_this());
}
std::shared_ptr<Symbol> ScopedSymbol::resolveType(const std::string &type) {
  for (const auto &[symName, symbol] : typeSymbols) {
    if (symName == type) {
      return symbol;
    }
  }
  if (const auto parent = getEnclosingScope()) {
    return parent->resolveType(type);
  }
  return nullptr;
}
std::shared_ptr<Symbol> ScopedSymbol::resolveSymbol(const std::string &name) {
  for (const auto &[symName, symbol] : symbols) {
    if (symName == name) {
      return symbol;
    }
  }
  if (const auto parent = getEnclosingScope()) {
    return parent->resolveSymbol(name);
  }
  return nullptr;
}
std::string ScopedSymbol::toString() {
  std::stringstream ss;
  ss << scTypeToString() << " { ";
  for (const auto &[name, symbol] : symbols) {
    ss << symbol->toString() << " ";
  }
  ss << "}";
  return ss.str();
}
} // namespace gazprea::symTable