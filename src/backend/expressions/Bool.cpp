#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBool(std::shared_ptr<ast::expressions::BoolLiteralAst> ctx) {
  auto value = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), ctx->getValue() ? 1 : 0);
  auto valueAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), boolTy(), constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, value, valueAddr);
  pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), valueAddr);
  return {};
}

} // namespace gazprea::backend
