#include "backend/Backend.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::backend {

std::any Backend::visitTupleAccess(std::shared_ptr<ast::expressions::TupleAccessAst> ctx) {
  auto tupleSym = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  auto tupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(tupleSym->getType());
  auto sTy = getMLIRType(tupleTypeSymbol);

  if (ctx->getSymbol()->getScope().lock()->getScopeType() == symTable::ScopeType::Global) {
    auto globalAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, ptrTy(), ctx->getTupleName());

    auto gepIndices = std::vector<mlir::Value>{
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(),
                                                ctx->getFieldIndex() - 1)};

    auto elementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), sTy, globalAddr, gepIndices);

    pushElementToScopeStack(ctx, tupleTypeSymbol->getResolvedTypes()[ctx->getFieldIndex() - 1],
                            elementPtr);

    return {};
  }

  auto structAddr = tupleSym->value;
  auto gepIndices = std::vector<mlir::Value>{
      builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
      builder->create<mlir::LLVM::ConstantOp>(
          loc, builder->getI32Type(), ctx->getFieldIndex() - 1)}; // Tuples are 1-based indexing
  auto elementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), sTy, structAddr, gepIndices);
  pushElementToScopeStack(ctx, tupleTypeSymbol->getResolvedTypes()[ctx->getFieldIndex() - 1],
                          elementPtr);
  return {};
}

} // namespace gazprea::backend
