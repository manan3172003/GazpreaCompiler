#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitArrayAccess(std::shared_ptr<ast::expressions::ArrayAccessAst> ctx) {
  visit(ctx->getArrayInstance());
  auto [arrayInstanceType, arrayInstanceAddr] = popElementFromStack(ctx);
  visit(ctx->getElementIndex());

  if (ctx->getElementIndex()->getNodeType() == ast::NodeType::SingularIndexExpr) {
    auto [indexType, indexAddr] = popElementFromStack(ctx);
    auto arraySizeAddr =
        getArraySizeAddr(*builder, loc, getMLIRType(arrayInstanceType), arrayInstanceAddr);
    auto arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);
    auto indexValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(indexType), indexAddr);
    auto normalizedIndex = normalizeIndex(indexValue, arraySize); // converted to 0-indexed form

    auto arrayStructType = getMLIRType(arrayInstanceType);
    auto dataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayInstanceAddr);
    mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

    auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayInstanceType);
    auto elementType = arrayTypeSym->getType();
    mlir::Type elementMLIRType = getMLIRType(elementType);

    // elementPtr = &dataPtr[normalizedIndex]
    auto elementPtrOp = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dataPtr,
                                                           mlir::ValueRange{normalizedIndex});
    mlir::Value elementPtr = elementPtrOp.getResult();

    if (ctx->isLValue()) {
      // LValue: store pointer directly
      pushElementToScopeStack(ctx, elementType, elementPtr);
    } else {
      // RValue: make deep copy
      auto tmpAlloca =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), elementMLIRType, constOne());
      copyValue(elementType, elementPtr, tmpAlloca);
      pushElementToScopeStack(ctx, elementType, tmpAlloca);
    }

  } else if (ctx->getElementIndex()->getNodeType() == ast::NodeType::RangedIndexExpr) {
    auto [rightIndexType, rightIndexAddr] = popElementFromStack(ctx);
    auto [leftIndexType, leftIndexAddr] = popElementFromStack(ctx);
  }

  return {};
}

} // namespace gazprea::backend