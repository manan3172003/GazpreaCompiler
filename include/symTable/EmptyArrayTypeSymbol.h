#pragma once
#include "ArrayTypeSymbol.h"
#include "Symbol.h"

namespace gazprea::symTable {

class EmptyArrayTypeSymbol final : public ArrayTypeSymbol {
public:
  explicit EmptyArrayTypeSymbol(const std::string &name) : ArrayTypeSymbol(name) {};
  std::string getName() override;
  std::string toString() override;
};
} // namespace gazprea::symTable
