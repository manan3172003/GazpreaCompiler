#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitOutput(std::shared_ptr<ast::statements::OutputAst> ctx) {
  visit(ctx->getExpression());

  auto [type, valueAddr] = ctx->getScope()->getTopElementInStack();

  auto value = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), valueAddr);

  printInt(value);
  return {};
}

} // namespace gazprea::backend
