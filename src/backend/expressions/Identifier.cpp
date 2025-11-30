#include "backend/Backend.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::backend {

std::any Backend::visitIdentifier(std::shared_ptr<ast::expressions::IdentifierAst> ctx) {
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (ctx->getSymbol()->getScope().lock()->getScopeType() == symTable::ScopeType::Global) {
    auto globalAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, ptrTy(), ctx->getName());
    if (variableSymbol->getQualifier() == ast::Qualifier::Const ||
        isTypeArray(ctx->getInferredSymbolType()) || isTypeVector(ctx->getInferredSymbolType())) {
      auto newAddr = builder->create<mlir::LLVM::AllocaOp>(
          loc, ptrTy(), getMLIRType(ctx->getInferredSymbolType()), constOne());
      copyValue(ctx->getInferredSymbolType(), globalAddr, newAddr);
      ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), newAddr);
      return {};
    }
    ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), globalAddr);
    return {};
  }
  const auto valueAddr = ctx->getSymbol()->value;
  auto variableSymType =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol())->getType();
  if (variableSymbol->getQualifier() == ast::Qualifier::Const || isTypeArray(variableSymType) ||
      isTypeVector(variableSymType)) {
    auto newAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(variableSymType),
                                                         constOne());
    copyValue(variableSymType, valueAddr, newAddr);
    ctx->getScope()->pushElementToScopeStack(variableSymType, newAddr);
    return {};
  }
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), valueAddr);
  return {};
}

} // namespace gazprea::backend
