#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBinary(std::shared_ptr<ast::expressions::BinaryAst> ctx) {
  visit(ctx->getLeft());
  visit(ctx->getRight());

  auto [leftType, leftAddr] = ctx->getRight()->getScope()->getSecondElementInStack();
  auto [rightType, rightAddr] = ctx->getLeft()->getScope()->getTopElementInStack();
  auto newAddr = binaryOperandToValue(
      ctx->getBinaryOpType(), ctx->getInferredSymbolType(), ctx->getLeft()->getInferredSymbolType(),
      ctx->getRight()->getInferredSymbolType(), leftAddr, rightAddr);
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), newAddr);
  return {};
}

} // namespace gazprea::backend
