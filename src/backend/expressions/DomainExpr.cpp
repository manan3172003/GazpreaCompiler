#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitDomainExpr(std::shared_ptr<ast::expressions::DomainExprAst> ctx) {
  visit(ctx->getDomainExpression());
  auto [original_type, original_addr] = popElementFromStack(ctx);

  auto copy_addr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(original_type), constOne());

  copyValue(original_type, original_addr, copy_addr);

  pushElementToScopeStack(ctx, original_type, copy_addr);

  return {};
}

} // namespace gazprea::backend
