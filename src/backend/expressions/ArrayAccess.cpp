#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/VectorTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitArrayAccess(std::shared_ptr<ast::expressions::ArrayAccessAst> ctx) {
  visit(ctx->getArrayInstance());
  auto [arrayInstanceType, arrayInstanceAddr] = popElementFromStack(ctx);
  visit(ctx->getElementIndex());

  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(arrayInstanceType);
  if (vectorTypeSym) {
    auto vectorStructType = getMLIRType(arrayInstanceType);
    auto elementType = vectorTypeSym->getType();

    if (ctx->getElementIndex()->getNodeType() == ast::NodeType::SingularIndexExpr) {
      auto sizeAddr = gepOpVector(vectorStructType, arrayInstanceAddr, VectorOffset::Size);
      auto vectorSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizeAddr);

      auto dataAddr = gepOpVector(vectorStructType, arrayInstanceAddr, VectorOffset::Data);
      auto dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

      handleSingularIndexAccess(ctx, elementType, vectorSize, dataPtr);
    } else if (ctx->getElementIndex()->getNodeType() == ast::NodeType::RangedIndexExpr) {
      auto sizeAddr = gepOpVector(vectorStructType, arrayInstanceAddr, VectorOffset::Size);
      auto vectorSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizeAddr);

      auto dataAddr = gepOpVector(vectorStructType, arrayInstanceAddr, VectorOffset::Data);
      auto dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

      handleRangedIndexAccess(ctx, arrayInstanceType, arrayInstanceAddr, vectorSize, dataPtr,
                              vectorStructType);
    }
    return {};
  }

  if (ctx->getElementIndex()->getNodeType() == ast::NodeType::SingularIndexExpr) {
    auto arrayStructType = getMLIRType(arrayInstanceType);
    auto arraySizeAddr =
        getArraySizeAddr(*builder, loc, getMLIRType(arrayInstanceType), arrayInstanceAddr);
    auto arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);

    auto dataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayInstanceAddr);
    mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

    auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayInstanceType);
    auto elementType = arrayTypeSym->getType();

    handleSingularIndexAccess(ctx, elementType, arraySize, dataPtr);

  } else if (ctx->getElementIndex()->getNodeType() == ast::NodeType::RangedIndexExpr) {
    auto arrayStructType = getMLIRType(arrayInstanceType);
    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayInstanceAddr);
    mlir::Value arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);

    auto dataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayInstanceAddr);
    mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

    handleRangedIndexAccess(ctx, arrayInstanceType, arrayInstanceAddr, arraySize, dataPtr,
                            arrayStructType);
  }

  return {};
}

void Backend::handleSingularIndexAccess(std::shared_ptr<ast::expressions::ArrayAccessAst> ctx,
                                        std::shared_ptr<symTable::Type> elementType,
                                        mlir::Value size, mlir::Value dataPtr) {
  auto [indexType, indexAddr] = popElementFromStack(ctx);
  auto indexValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(indexType), indexAddr);
  auto normalizedIndex = normalizeIndex(indexValue, size); // converted to 0-indexed form

  auto elementMLIRType = getMLIRType(elementType);

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
}

void Backend::handleRangedIndexAccess(std::shared_ptr<ast::expressions::ArrayAccessAst> ctx,
                                      std::shared_ptr<symTable::Type> instanceType,
                                      mlir::Value instanceAddr, mlir::Value size,
                                      mlir::Value dataPtr, mlir::Type structType) {
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

  auto leftVal = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(leftIndexType), leftIndexAddr)
                     .getResult();

  mlir::Value normLeft = normalizeIndex(leftVal, size);

  mlir::Value normRight;
  if (hasRight) {
    auto rightVal =
        builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(rightIndexType), rightIndexAddr)
            .getResult();
    normRight = normalizeIndex(rightVal, size);
  } else {
    normRight = size;
  }

  auto sliceSizeOp = builder->create<mlir::LLVM::SubOp>(loc, intTy(), normRight, normLeft);
  mlir::Value sliceSize = sliceSizeOp.getResult();

  // Bounds checks:
  // normLeft <= normRight  (we require non-negative sliceSize)
  auto isSliceNeg =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, sliceSize,
                                          builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0));

  // ensure throwIndexError exists (declare if missing)
  auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
      "throwArrayIndexError_019ae3a1_54f9_7452_b095_6faaebe8aa2e");

  // If out of range -> call throwIndexError (then yield nothing). We do this as a scf::If with no
  // results.
  builder->create<mlir::scf::IfOp>(loc, isSliceNeg.getResult(),
                                   [&](mlir::OpBuilder &b, mlir::Location l) {
                                     b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
                                     b.create<mlir::scf::YieldOp>(l);
                                   });

  // Now create the slice array struct (an alloca for the array struct so we can return its
  // address).
  auto sliceStructAlloca =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), structType, constOne());

  mlir::Value sliceDataAddr, sliceSizeAddr, sliceIs2DAddr, sliceCapacityAddr;
  mlir::Value origIs2D;

  bool isVector = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(instanceType) != nullptr;

  if (isVector) {
    sliceSizeAddr = gepOpVector(structType, sliceStructAlloca, VectorOffset::Size);
    sliceCapacityAddr = gepOpVector(structType, sliceStructAlloca, VectorOffset::Capacity);
    sliceDataAddr = gepOpVector(structType, sliceStructAlloca, VectorOffset::Data);
    sliceIs2DAddr = gepOpVector(structType, sliceStructAlloca, VectorOffset::Is2D);

    auto origIs2DAddr = gepOpVector(structType, instanceAddr, VectorOffset::Is2D);
    origIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), origIs2DAddr);

    builder->create<mlir::LLVM::StoreOp>(loc, sliceSize, sliceCapacityAddr);
  } else {
    sliceSizeAddr = getArraySizeAddr(*builder, loc, structType, sliceStructAlloca);
    sliceDataAddr = getArrayDataAddr(*builder, loc, structType, sliceStructAlloca);
    sliceIs2DAddr = get2DArrayBoolAddr(*builder, loc, structType, sliceStructAlloca);

    auto origIs2DAddr = get2DArrayBoolAddr(*builder, loc, structType, instanceAddr);
    origIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), origIs2DAddr);
  }

  builder->create<mlir::LLVM::StoreOp>(loc, sliceSize, sliceSizeAddr);
  builder->create<mlir::LLVM::StoreOp>(loc, origIs2D, sliceIs2DAddr);

  std::shared_ptr<symTable::Type> elementType;
  if (isVector) {
    elementType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(instanceType)->getType();
  } else {
    elementType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(instanceType)->getType();
  }
  mlir::Type elementMLIRType = getMLIRType(elementType);

  if (ctx->isLValue()) {
    // LValue semantics: point into original data (no copy).
    // compute srcStart = &dataPtr[normLeft]
    auto srcStartOp = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dataPtr,
                                                         mlir::ValueRange{normLeft});
    mlir::Value srcStart = srcStartOp.getResult();

    // store srcStart into slice struct data field
    builder->create<mlir::LLVM::StoreOp>(loc, srcStart, sliceDataAddr);
    pushElementToScopeStack(ctx, instanceType, sliceStructAlloca);
  } else {
    // RValue semantics: allocate new buffer and copy slice
    // srcStart pointer (start of slice in original data)
    auto srcStartOp = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dataPtr,
                                                         mlir::ValueRange{normLeft});
    mlir::Value srcStart = srcStartOp.getResult();

    if (isVector) {
      // Allocate new data buffer
      auto newDataPtr = mallocArray(elementMLIRType, sliceSize);
      builder->create<mlir::LLVM::StoreOp>(loc, newDataPtr, sliceDataAddr);

      builder->create<mlir::scf::ForOp>(
          loc, constZero(), sliceSize, constOne(), mlir::ValueRange{},
          [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
            auto srcElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, srcStart,
                                                          mlir::ValueRange{i});
            auto destElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                           mlir::ValueRange{i});
            copyValue(elementType, srcElemPtr, destElemPtr);
            b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
          });
    } else {
      // allocate and copy data: use copyArray(elementType, srcStart, sliceSize)
      // copyArray returns a dest data pointer (malloc'ed)
      mlir::Value destDataPtr = copyArray(elementType, srcStart, sliceSize);
      builder->create<mlir::LLVM::StoreOp>(loc, destDataPtr, sliceDataAddr);
    }

    pushElementToScopeStack(ctx, instanceType, sliceStructAlloca);
  }
}

} // namespace gazprea::backend