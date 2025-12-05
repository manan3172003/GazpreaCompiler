#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitInteger(std::shared_ptr<ast::expressions::IntegerLiteralAst> ctx) {
  auto value = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), ctx->integerValue);
  auto valueAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, value, valueAddr);
  pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), valueAddr);
  return {};
}

} // namespace gazprea::backend
