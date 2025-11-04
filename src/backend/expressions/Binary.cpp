#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBinary(std::shared_ptr<ast::expressions::BinaryAst> ctx) {
  visit(ctx->getLeft());
  auto [leftType, leftAddr] = ctx->getScope()->getTopElementInStack();
  visit(ctx->getRight());
  auto [rightType, rightAddr] = ctx->getScope()->getTopElementInStack();

  ctx->getScope()->popElementFromScopeStack(); // Pop right operand
  ctx->getScope()->popElementFromScopeStack(); // Pop left operand

  auto newAddr = binaryOperandToValue(
      ctx->getBinaryOpType(), ctx->getInferredSymbolType(), ctx->getLeft()->getInferredSymbolType(),
      ctx->getRight()->getInferredSymbolType(), leftAddr, rightAddr);
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), newAddr);
  return {};
}

} // namespace gazprea::backend
