#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitChar(std::shared_ptr<ast::expressions::CharLiteralAst> ctx) {
  auto value = builder->create<mlir::LLVM::ConstantOp>(
      loc, charTy(), builder->getIntegerAttr(charTy(), ctx->getValue()));
  auto valueAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), charTy(), constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, value, valueAddr);
  pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), valueAddr);
  return {};
}

} // namespace gazprea::backend
