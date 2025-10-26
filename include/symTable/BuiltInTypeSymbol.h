#pragma once
#include "Symbol.h"

namespace gazprea::symTable {
class BuiltInTypeSymbol final : public Symbol, public Type {
public:
  explicit BuiltInTypeSymbol(const std::string &name) : Symbol(name), Type() {};
  std::string getName() override;
  std::string toString() override;
};
} // namespace gazprea::symTable