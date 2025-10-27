#include <sstream>
#include <symTable/Scope.h>

namespace gazprea::symTable {
Scope::~Scope() = default;
void BaseScope::setEnclosingScope(std::shared_ptr<Scope> scope) {
  enclosingScope = scope;
}
std::shared_ptr<Scope> BaseScope::getEnclosingScope() {
  return enclosingScope.lock();
}
void BaseScope::define(std::shared_ptr<Symbol> sym) {
  symbols.emplace(sym->getName(), sym);
  sym->setScope(shared_from_this());
}
std::shared_ptr<Symbol> BaseScope::resolve(const std::string &name) {
  if (const auto it = symbols.find(name); it != symbols.end()) {
    return it->second;
  }
  if (const auto parent = getEnclosingScope()) {
    return parent->resolve(name);
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