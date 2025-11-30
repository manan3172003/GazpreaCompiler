#include "backend/Backend.h"
#include "symTable/StructTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitStructAccess(std::shared_ptr<ast::expressions::StructAccessAst> ctx) {
  const auto structSym = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  const auto structTypeSymbol =
      std::dynamic_pointer_cast<symTable::StructTypeSymbol>(structSym->getType());
  auto sTy = getMLIRType(structTypeSymbol);

  auto structAddr = structSym->value;
  auto gepIndices = std::vector<mlir::Value>{
      builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
      builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(),
                                              structTypeSymbol->getIdx(ctx->getElementName()) -
                                                  1)}; // Structs are 1-based indexing
  auto elementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), sTy, structAddr, gepIndices);
  ctx->getScope()->pushElementToScopeStack(structTypeSymbol->getResolvedType(ctx->getElementName()),
                                           elementPtr);
  return {};
}

} // namespace gazprea::backend