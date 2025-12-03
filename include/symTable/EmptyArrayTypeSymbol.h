#pragma once
#include "Symbol.h"
#include "Type.h"

namespace gazprea::symTable {

class EmptyArrayTypeSymbol : public Type, public Symbol {
public:
  explicit EmptyArrayTypeSymbol(const std::string &name) : Symbol(name) {};
  std::string getName() override;
  std::string toString() override;
};
} // namespace gazprea::symTable
