#pragma once
#include "Symbol.h"

namespace gazprea::symTable {
class TypealiasSymbol final : public Symbol, public Type {
  std::string name;
  std::shared_ptr<Type> type;

public:
  explicit TypealiasSymbol(const std::string &name) : Symbol(name) {}
  std::string getName() override { return name; }
  void setType(std::shared_ptr<Type> &type_) { type = type_; }
  std::shared_ptr<Type> getType() { return type; }
};
} // namespace gazprea::symTable