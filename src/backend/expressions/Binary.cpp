#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBinary(std::shared_ptr<ast::expressions::BinaryAst> ctx) {
  visit(ctx->getLeft());
  auto [leftType, leftAddr] = popElementFromStack(ctx);
  visit(ctx->getRight());
  auto [rightType, rightAddr] = popElementFromStack(ctx);

  auto newAddr =
      binaryOperandToValue(ctx, ctx->getBinaryOpType(), ctx->getInferredSymbolType(),
                           ctx->getLeft()->getInferredSymbolType(),
                           ctx->getRight()->getInferredSymbolType(), leftAddr, rightAddr);
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), newAddr);
  return {};
}

} // namespace gazprea::backend
