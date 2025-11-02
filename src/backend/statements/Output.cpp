#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitOutput(std::shared_ptr<ast::statements::OutputAst> ctx) {
  visit(ctx->getExpression());

  auto topElement = ctx->getScope()->getTopElementInStack();
  auto valueAddr = topElement.second;

  assert(valueAddr); // Addr to the value from the top of the stack should not be null
  auto value = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), valueAddr);

  printInt(value);
  return {};
}

} // namespace gazprea::backend
