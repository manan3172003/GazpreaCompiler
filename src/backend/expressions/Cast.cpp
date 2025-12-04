#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitCast(std::shared_ptr<ast::expressions::CastAst> ctx) {
  visit(ctx->getExpression());
  auto [exprType, exprAddr] = popElementFromStack(ctx);

  const auto targetType = ctx->getResolvedTargetType();
  auto targetMlirType = getMLIRType(targetType);
  auto targetAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), targetMlirType, constOne());

  performExplicitCast(exprAddr, exprType, targetAddr, targetType);
  pushElementToScopeStack(ctx, targetType, targetAddr);

  return {};
}

} // namespace gazprea::backend
