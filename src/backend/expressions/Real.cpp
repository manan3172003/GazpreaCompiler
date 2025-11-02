#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitReal(std::shared_ptr<ast::expressions::RealLiteralAst> ctx) {
  auto value = builder->create<mlir::LLVM::ConstantOp>(
      loc, floatTy(), builder->getFloatAttr(floatTy(), ctx->realValue));
  auto valueAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), floatTy(), constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, value, valueAddr);
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), valueAddr);
  return {};
}

} // namespace gazprea::backend
