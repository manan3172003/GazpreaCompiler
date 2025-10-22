#include <symTable/Symbol.h>

namespace gazprea::symTable {
Symbol::Symbol(const std::string &name) {}
Symbol::Symbol(std::string name, std::shared_ptr<types::Type> type) {}
Symbol::Symbol(std::string name, std::shared_ptr<types::Type> type,
               std::shared_ptr<Scope> scope) {}
std::string Symbol::getName() {}
std::string Symbol::toString() {}
} // namespace gazprea::symTable