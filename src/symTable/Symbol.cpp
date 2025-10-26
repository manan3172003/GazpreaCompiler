#include <symTable/Symbol.h>

namespace gazprea::symTable {
Symbol::Symbol(const std::string &name) {}
Symbol::Symbol(std::string name, std::shared_ptr<symTable::Type> type) {}
Symbol::Symbol(std::string name, std::shared_ptr<symTable::Type> type,
               std::shared_ptr<Scope> scope) {}

std::weak_ptr<Scope> Symbol::getScope() { return scope; }
std::string Symbol::getName() { return name; }
std::string Symbol::toString() {
  const std::string sname =
      (!scope.expired()) ? scope.lock()->getScopeName() + "::" : "";
  return '<' + sname + name + '>';
}
} // namespace gazprea::symTable