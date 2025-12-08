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

  // For BY operation on vectors, the result is an array type
  auto resultType = ctx->getInferredSymbolType();
  if (ctx->getBinaryOpType() == ast::expressions::BinaryOpType::BY &&
      isTypeVector(ctx->getLeft()->getInferredSymbolType())) {
    auto vectorType =
        std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(ctx->getInferredSymbolType());
    if (vectorType) {
      auto arrayType = std::make_shared<symTable::ArrayTypeSymbol>("array");
      arrayType->setType(vectorType->getType());
      resultType = arrayType;
    }
  }
  pushElementToScopeStack(ctx, resultType, newAddr);
  return {};
}

} // namespace gazprea::backend
