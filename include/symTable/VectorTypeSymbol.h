#pragma once
#include "Symbol.h"
#include "Type.h"

namespace gazprea::symTable {
class VectorTypeSymbol final : public Type, public Symbol {
  std::shared_ptr<Type> type;

public:
  int inferredSize = 0; // number of elements in the vector, remains same for declared and inferred.
  std::vector<int> inferredElementSize;
  std::vector<mlir::Value> declaredElementSize;
  std::vector<bool> elementSizeInferenceFlags;
  bool isScalar = true;
  bool isElement2D = false;

  explicit VectorTypeSymbol(const std::string &name) : Symbol(name) {};
  std::string getName() override;
  std::string toString() override;

  void setType(const std::shared_ptr<Type> &_type) { type = _type; }
  std::shared_ptr<Type> getType() { return type; }
  void setElementSizeInferenceFlags(const std::vector<bool> &flags) {
    elementSizeInferenceFlags = flags;
  }
  const std::vector<bool> &getElementSizeInferenceFlags() const {
    return elementSizeInferenceFlags;
  }
};
} // namespace gazprea::symTable
