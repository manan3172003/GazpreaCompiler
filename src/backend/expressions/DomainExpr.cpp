#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitDomainExpr(std::shared_ptr<ast::expressions::DomainExprAst> ctx) {
  auto innerExpr = ctx->getDomainExpression();
  visit(innerExpr);

  auto scope = ctx->getScope();
  auto [original_type, original_addr] = scope->getTopElementInStack();
  scope->popElementFromScopeStack();

  auto copy_addr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(original_type), constOne());

  copyValue(original_type, original_addr, copy_addr);

  if (!innerExpr->isLValue()) {
    freeAllocatedMemory(original_type, original_addr);
  }

  pushElementToScopeStack(ctx, original_type, copy_addr);

  return {};
}

} // namespace gazprea::backend
