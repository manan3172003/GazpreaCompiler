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

    std::shared_ptr<symTable::Type> rightIndexType;
    mlir::Value rightIndexAddr;
    bool hasRight = false;
    if (std::dynamic_pointer_cast<ast::expressions::RangedIndexExprAst>(ctx->getElementIndex())
            ->getRightIndexExpr()) {
      auto [tempRightIndexType, tempRightIndexAddr] = popElementFromStack(ctx);
      rightIndexType = tempRightIndexType;
      rightIndexAddr = tempRightIndexAddr;
      hasRight = true;
    }

    auto [leftIndexType, leftIndexAddr] = popElementFromStack(ctx);

    auto leftVal =
        builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(leftIndexType), leftIndexAddr)
            .getResult();

    auto arrayStructType = getMLIRType(arrayInstanceType);
    auto dataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayInstanceAddr);
    mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayInstanceAddr);
    mlir::Value arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);

    mlir::Value normLeft = normalizeIndex(leftVal, arraySize);

    mlir::Value normRight;
    if (hasRight) {
      auto rightVal =
          builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(rightIndexType), rightIndexAddr)
              .getResult();
      normRight = normalizeIndex(rightVal, arraySize);
    } else {
      normRight = arraySize;
    }

    auto sliceSizeOp = builder->create<mlir::LLVM::SubOp>(loc, intTy(), normRight, normLeft);
    mlir::Value sliceSize = sliceSizeOp.getResult();

    // Bounds checks:
    // normLeft <= normRight  (we require non-negative sliceSize)
    auto isSliceNeg = builder->create<mlir::LLVM::ICmpOp>(
        loc, mlir::LLVM::ICmpPredicate::slt, sliceSize,
        builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0));

    // ensure throwIndexError exists (declare if missing)
    auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
        "throwArrayIndexError_019ae3a1_54f9_7452_b095_6faaebe8aa2e");

    // If out of range -> call throwIndexError (then yield nothing). We do this as a scf::If with no
    // results.
    builder->create<mlir::scf::IfOp>(
        loc, isSliceNeg.getResult(), [&](mlir::OpBuilder &b, mlir::Location l) {
          b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
          b.create<mlir::scf::YieldOp>(l);
        });

    // Now create the slice array struct (an alloca for the array struct so we can return its
    // address).
    auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayInstanceType);
    auto elementType = arrayTypeSym->getType();
    mlir::Type elementMLIRType = getMLIRType(elementType);

    auto sliceArrayStructType = getMLIRType(arrayInstanceType);
    auto sliceStructAlloca =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), sliceArrayStructType, constOne());

    // Pointers to fields inside the new struct
    auto sliceDataAddr = getArrayDataAddr(*builder, loc, sliceArrayStructType, sliceStructAlloca);
    auto sliceSizeAddr = getArraySizeAddr(*builder, loc, sliceArrayStructType, sliceStructAlloca);
    auto sliceIs2DAddr = get2DArrayBoolAddr(*builder, loc, sliceArrayStructType, sliceStructAlloca);

    if (ctx->isLValue()) {
      // LValue semantics: point into original data (no copy).
      // compute srcStart = &dataPtr[normLeft]
      auto srcStartOp = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dataPtr,
                                                           mlir::ValueRange{normLeft});
      mlir::Value srcStart = srcStartOp.getResult();

      // store srcStart into slice struct data field
      builder->create<mlir::LLVM::StoreOp>(loc, srcStart, sliceDataAddr);
      // store slice size
      builder->create<mlir::LLVM::StoreOp>(loc, sliceSize, sliceSizeAddr);

      // copy is2D flag from original array
      auto origIs2DAddr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayInstanceAddr);
      mlir::Value origIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), origIs2DAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, origIs2D, sliceIs2DAddr);

      // push the address of this slice struct as the evaluated result
      pushElementToScopeStack(ctx, arrayInstanceType, sliceStructAlloca);
    } else {
      // RValue semantics: allocate new buffer and copy slice
      // srcStart pointer (start of slice in original data)
      auto srcStartOp = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dataPtr,
                                                           mlir::ValueRange{normLeft});
      mlir::Value srcStart = srcStartOp.getResult();

      // allocate and copy data: use copyArray(elementType, srcStart, sliceSize)
      // copyArray returns a dest data pointer (malloc'ed)
      mlir::Value destDataPtr = copyArray(elementType, srcStart, sliceSize);

      // store destDataPtr into slice struct and size
      builder->create<mlir::LLVM::StoreOp>(loc, destDataPtr, sliceDataAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, sliceSize, sliceSizeAddr);

      // copy is2D flag from original (if needed)
      auto origIs2DAddr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayInstanceAddr);
      mlir::Value origIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), origIs2DAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, origIs2D, sliceIs2DAddr);

      // push the address of this newly-owned slice struct
      pushElementToScopeStack(ctx, arrayInstanceType, sliceStructAlloca);
    }
  }

  return {};
}

} // namespace gazprea::backend