#include "backend/Backend.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::backend {

std::any Backend::visitIdentifier(std::shared_ptr<ast::expressions::IdentifierAst> ctx) {
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());

  // Check if this is a generator iterator variable (stored in blockArg map)
  if (blockArg.find(ctx->getName()) != blockArg.end()) {
    auto iteratorAddr = blockArg[ctx->getName()];
    auto variableSymType = variableSymbol->getType();
    if (variableSymbol->getQualifier() == ast::Qualifier::Const) {
      auto newAddr = builder->create<mlir::LLVM::AllocaOp>(
          loc, ptrTy(), getMLIRType(variableSymType), constOne());
      copyValue(variableSymType, iteratorAddr, newAddr);
      pushElementToScopeStack(ctx, variableSymType, newAddr);
      return {};
    }
    pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), iteratorAddr);
    return {};
  }

  if (ctx->getSymbol()->getScope().lock()->getScopeType() == symTable::ScopeType::Global) {
    auto globalAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, ptrTy(), ctx->getName());

    if (not ctx->isLValue()) {
      auto newAddr = builder->create<mlir::LLVM::AllocaOp>(
          loc, ptrTy(), getMLIRType(ctx->getInferredSymbolType()), constOne());
      copyValue(ctx->getInferredSymbolType(), globalAddr, newAddr);
      pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), newAddr);
    } else {
      pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), globalAddr);
    }
    return {};
  }

  const auto valueAddr = ctx->getSymbol()->value;
  auto variableSymType =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol())->getType();

  if (not ctx->isLValue()) {
    auto newAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(variableSymType),
                                                         constOne());
    copyValue(variableSymType, valueAddr, newAddr);
    pushElementToScopeStack(ctx, variableSymType, newAddr);
  } else {
    pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), valueAddr);
  }
  return {};
}

} // namespace gazprea::backend
