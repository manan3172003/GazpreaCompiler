#pragma once
#include "Symbol.h"
#include "Type.h"

namespace gazprea::symTable {
class VectorTypeSymbol final : public Type, public Symbol {
  std::shared_ptr<Type> type;

public:
  explicit VectorTypeSymbol(const std::string &name) : Symbol(name) {};
  std::string getName() override;
  std::string toString() override;

  void setType(const std::shared_ptr<Type> &_type) { type = _type; }
  std::shared_ptr<Type> getType() { return type; }
};
} // namespace gazprea::symTable