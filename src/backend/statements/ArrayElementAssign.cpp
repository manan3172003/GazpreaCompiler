#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"

namespace gazprea::backend {

std::any
Backend::visitArrayElementAssign(std::shared_ptr<ast::statements::ArrayElementAssignAst> ctx) {
  visit(ctx->getArrayInstance());
  const auto leftMlirType = getMLIRType(ctx->getArrayInstance()->getAssignSymbolType());

  visit(ctx->getElementIndex());
  if (ctx->getElementIndex()->getNodeType() == ast::NodeType::SingularIndexExpr) {
    auto [indexType, indexAddr] = popElementFromStack(ctx);
    auto arraySizeAddr =
        getArraySizeAddr(*builder, loc, leftMlirType, ctx->getArrayInstance()->getEvaluatedAddr());
    auto arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);
    auto indexValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(indexType), indexAddr);
    auto normalizedIndex = normalizeIndex(indexValue, arraySize); // converted to 0-indexed form
    auto dataAddr =
        getArrayDataAddr(*builder, loc, leftMlirType, ctx->getArrayInstance()->getEvaluatedAddr());
    mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

    auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(
        ctx->getArrayInstance()->getAssignSymbolType());
    auto elementType = arrayTypeSym->getType();
    mlir::Type elementMLIRType = getMLIRType(elementType);

    // elementPtr = &dataPtr[normalizedIndex]
    auto elementPtrOp = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dataPtr,
                                                           mlir::ValueRange{normalizedIndex});
    mlir::Value elementPtr = elementPtrOp.getResult();
    ctx->setEvaluatedAddr(elementPtr);

  } else if (ctx->getElementIndex()->getNodeType() == ast::NodeType::RangedIndexExpr) {
    auto [rightIndexType, rightIndexAddr] = popElementFromStack(ctx);
    auto [leftIndexType, leftIndexAddr] = popElementFromStack(ctx);
  }

  return {};
}

} // namespace gazprea::backend