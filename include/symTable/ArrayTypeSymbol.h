#pragma once
#include "Symbol.h"
#include "Type.h"

namespace gazprea::symTable {

class ArrayTypeSymbol : public Type, public Symbol {
  std::shared_ptr<Type> type;
  std::vector<mlir::Value> sizes;

public:
  explicit ArrayTypeSymbol(const std::string &name) : Symbol(name) {};
  std::string getName() override;
  std::string toString() override;

  void setType(std::shared_ptr<Type> _type) { type = _type; }
  std::shared_ptr<Type> getType() { return type; }

  void addSize(mlir::Value size) { sizes.push_back(size); }
  std::vector<mlir::Value> getSizes() { return sizes; }
};
} // namespace gazprea::symTable