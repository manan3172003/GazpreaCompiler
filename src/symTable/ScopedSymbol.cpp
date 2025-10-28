#include <sstream>
#include <symTable/ScopedSymbol.h>

namespace gazprea::symTable {
std::string ScopedSymbol::getScopeName() { return "ScopedSymbol"; }
void ScopedSymbol::setEnclosingScope(std::shared_ptr<Scope> scope) {
  enclosingScope = scope;
}
std::shared_ptr<Scope> ScopedSymbol::getEnclosingScope() {
  return enclosingScope.lock();
}
void ScopedSymbol::defineSymbol(std::shared_ptr<Symbol> sym) {
  symbols.emplace_back(sym->getName(), sym);
  sym->setScope(shared_from_this());
}
void ScopedSymbol::defineTypeSymbol(std::shared_ptr<Symbol> sym) {
  typeSymbols.emplace_back(sym->getName(), sym);
  sym->setScope(shared_from_this());
}
std::shared_ptr<Symbol> ScopedSymbol::resolve(const std::string &name) {
  for (const auto &[symName, symbol] : symbols) {
    if (symName == name) {
      return symbol;
    }
  }
  if (const auto parent = getEnclosingScope()) {
    return parent->resolve(name);
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