#include "symTable/BuiltInTypeSymbol.h"

#include <sstream>
#include <symTable/SymTable.h>

namespace gazprea::symTable {
void SymbolTable::initTypeSystem() const {
  globalScope->defineTypeSymbol(std::make_shared<BuiltInTypeSymbol>("integer"));
  globalScope->defineTypeSymbol(std::make_shared<BuiltInTypeSymbol>("real"));
  globalScope->defineTypeSymbol(std::make_shared<BuiltInTypeSymbol>("character"));
  globalScope->defineTypeSymbol(std::make_shared<BuiltInTypeSymbol>("boolean"));

  // TODO: Add more built-in types for part 2
}
SymbolTable::SymbolTable() {
  globalScope = std::make_shared<GlobalScope>();
  initTypeSystem();
  currentScope = getGlobalScope();
}
void SymbolTable::pushScope(std::shared_ptr<Scope> child) {
  child->setEnclosingScope(currentScope);
  currentScope = child;
}
void SymbolTable::popScope() {
  if (const auto parent = currentScope->getEnclosingScope()) {
    currentScope = parent;
  }
}
std::shared_ptr<GlobalScope> SymbolTable::getGlobalScope() { return globalScope; }
void SymbolTable::setGlobalScope(std::shared_ptr<GlobalScope> scope) { globalScope = scope; }
std::shared_ptr<Scope> SymbolTable::getCurrentScope() { return currentScope; }
void SymbolTable::setCurrentScope(std::shared_ptr<Scope> scope) { currentScope = scope; }
std::string SymbolTable::toString() const {
  std::stringstream ss;
  ss << "SymbolTable {\n";
  ss << "\tGlobal Scope { " << globalScope->toString() << " }\n";
  ss << "\tCurrent Scope { " << currentScope->toString() << " }\n";
  ss << "}\n";

  return ss.str();
}
} // namespace gazprea::symTable