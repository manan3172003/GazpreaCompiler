#pragma once
#include "Symbol.h"

namespace gazprea::symTable {
class VariableSymbol final : public Symbol {
public:
  explicit VariableSymbol(const std::string &name) : Symbol(name) {};
  std::string getName() override;
  std::string toString() override;
};
} // namespace gazprea::symTable