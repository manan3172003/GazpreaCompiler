#include <symTable/Symbol.h>

namespace gazprea::symTable {
Symbol::Symbol(const std::string &name) : name(name) {}

std::weak_ptr<Scope> Symbol::getScope() { return scope; }
std::string Symbol::getName() { return name; }
std::string Symbol::toString() {
  const std::string sname =
      (!scope.expired()) ? scope.lock()->getScopeName() + "::" : "";
  return '<' + sname + name + '>';
}
} // namespace gazprea::symTable