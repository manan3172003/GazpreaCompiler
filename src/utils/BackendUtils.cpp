#include <utils/BackendUtils.h>

namespace gazprea::utils {
mlir::Type getMLIRTypeFromSymbolType(mlir::MLIRContext &context,
                                     const std::shared_ptr<symTable::Type> &typeSymbol) {
  if (typeSymbol->getName() == "integer") {
    return mlir::IntegerType::get(&context, 32);
  }
  if (typeSymbol->getName() == "real") {
    return mlir::Float32Type::get(&context);
  }
  if (typeSymbol->getName() == "character") {
    return mlir::IntegerType::get(&context, 8);
  }
  if (typeSymbol->getName() == "boolean") {
    return mlir::IntegerType::get(&context, 1);
  }
  // TODO: handle Tuple types
}
} // namespace gazprea::utils