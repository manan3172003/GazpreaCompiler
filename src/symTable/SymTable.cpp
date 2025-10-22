#include <symTable/SymTable.h>

namespace gazprea::symTable {
void SymbolTable::initTypeSystem() const {}
SymbolTable::SymbolTable() {}
void SymbolTable::pushScope(std::shared_ptr<Scope> child) {}
void SymbolTable::popScope() {}
std::string SymbolTable::toString() const {}
} // namespace gazprea::symTable