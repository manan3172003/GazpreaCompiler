#include "backend/Backend.h"
#include "symTable/StructTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitStruct(std::shared_ptr<ast::expressions::StructLiteralAst> ctx) {
  const auto structTypeSymbol =
      std::dynamic_pointer_cast<symTable::StructTypeSymbol>(ctx->getInferredSymbolType());
  const auto structType = getMLIRType(structTypeSymbol);
  auto structAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), structType, constOne());

  for (size_t i = 0; i < ctx->getElements().size(); i++) {
    visit(ctx->getElements()[i]);
    auto [elementType, elementValueAddr] = popElementFromStack(ctx->getElements()[i]);
    auto gepIndices = std::vector<mlir::Value>{
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i)};
    auto elementPtr =
        builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), structType, structAddr, gepIndices);

    elementValueAddr =
        castIfNeeded(ctx, elementValueAddr, ctx->getElements()[i]->getInferredSymbolType(),
                     structTypeSymbol->getResolvedTypes()[i]);
    copyValue(structTypeSymbol->getResolvedTypes()[i], elementValueAddr, elementPtr);
    freeAllocatedMemory(structTypeSymbol->getResolvedTypes()[i], elementValueAddr);
  }
  pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), structAddr);
  return {};
}

} // namespace gazprea::backend