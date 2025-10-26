#pragma once
#include "Symbol.h"

namespace gazprea::symTable {
class TypealiasSymbol final : public Symbol, public Type {
  std::string name;
  Type type;

public:
  explicit TypealiasSymbol(const std::string &name) : Symbol(name) {}
  std::string getName() override { return name; }
  void setType(const Type &type_) { type = type_; }
  Type getType() { return type; }
};
} // namespace gazprea::symTable