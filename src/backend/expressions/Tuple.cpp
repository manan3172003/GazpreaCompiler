#include "backend/Backend.h"
#include "symTable/TupleTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitTuple(std::shared_ptr<ast::expressions::TupleLiteralAst> ctx) {
  auto sTy = getMLIRType(ctx->getInferredSymbolType());
  auto structAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), sTy, constOne());
  for (size_t i = 0; i < ctx->getElements().size(); i++) {
    visit(ctx->getElements()[i]);
    auto [elementType, elementValueAddr] =
        ctx->getElements()[i]->getScope()->getTopElementInStack();

    auto gepIndices = std::vector<mlir::Value>{
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i)};
    auto elementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), sTy, structAddr, gepIndices);

    auto elementValue =
        builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(elementType), elementValueAddr);

    builder->create<mlir::LLVM::StoreOp>(loc, elementValue, elementPtr);
  }
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), structAddr);
  return {};
}

} // namespace gazprea::backend
