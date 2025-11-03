#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitIdentifier(std::shared_ptr<ast::expressions::IdentifierAst> ctx) {
  if (ctx->getSymbol()->getScope().lock()->getScopeType() == symTable::ScopeType::Global) {
    auto globalAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, ptrTy(), ctx->getName());
    ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), globalAddr);
    return {};
  }
  const auto value = ctx->getSymbol()->value;
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), value);
  return {};
}

} // namespace gazprea::backend
