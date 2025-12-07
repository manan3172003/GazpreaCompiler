#include "backend/Backend.h"
#include "symTable/TupleTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitTuple(std::shared_ptr<ast::expressions::TupleLiteralAst> ctx) {
  const auto tupleTypeSymbol =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(ctx->getInferredSymbolType());
  const auto tupleType = getMLIRType(tupleTypeSymbol);
  auto tupleAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), tupleType, constOne());

  for (size_t i = 0; i < ctx->getElements().size(); i++) {
    visit(ctx->getElements()[i]);
    auto [elementType, elementValueAddr] = popElementFromStack(ctx->getElements()[i]);

    auto gepIndices = std::vector<mlir::Value>{
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i)};
    auto elementPtr =
        builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), tupleType, tupleAddr, gepIndices);

    elementValueAddr =
        castIfNeeded(ctx, elementValueAddr, ctx->getElements()[i]->getInferredSymbolType(),
                     tupleTypeSymbol->getResolvedTypes()[i]);
    copyValue(tupleTypeSymbol->getResolvedTypes()[i], elementValueAddr, elementPtr);
    freeAllocatedMemory(tupleTypeSymbol->getResolvedTypes()[i], elementValueAddr);
  }

  pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), tupleAddr);
  return {};
}

} // namespace gazprea::backend