#pragma once
#include "Symbol.h"
#include "ast/Ast.h"

namespace gazprea::symTable {
class VariableSymbol final : public Symbol {
  ast::Qualifier qualifier;

public:
  explicit VariableSymbol(const std::string &name,
                          const ast::Qualifier qualifier)
      : Symbol(name), qualifier(qualifier) {};

  ast::Qualifier getQualifier() const { return qualifier; }
  void setQualifier(const ast::Qualifier qual) { qualifier = qual; }

  std::string getName() override;
  std::string toString() override;
};
} // namespace gazprea::symTable