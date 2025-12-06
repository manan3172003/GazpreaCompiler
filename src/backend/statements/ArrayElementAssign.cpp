#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/VectorTypeSymbol.h"

namespace gazprea::backend {

std::any
Backend::visitArrayElementAssign(std::shared_ptr<ast::statements::ArrayElementAssignAst> ctx) {
  visit(ctx->getArrayInstance());
  const auto leftInstanceType = ctx->getArrayInstance()->getAssignSymbolType();
  const auto leftMlirType = getMLIRType(leftInstanceType);
  const bool isVector =
      static_cast<bool>(std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftInstanceType));

  visit(ctx->getElementIndex());
  if (ctx->getElementIndex()->getNodeType() == ast::NodeType::SingularIndexExpr) {
    auto [indexType, indexAddr] = popElementFromStack(ctx->getElementIndex());
    mlir::Value collectionSize;
    mlir::Value dataPtr;
    std::shared_ptr<symTable::Type> elementType;
    mlir::Type elementMLIRType;

    if (isVector) {
      auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftInstanceType);
      auto sizeAddr = gepOpVector(leftMlirType, ctx->getArrayInstance()->getEvaluatedAddr(),
                                  VectorOffset::Size);
      collectionSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizeAddr);
      auto dataAddr = gepOpVector(leftMlirType, ctx->getArrayInstance()->getEvaluatedAddr(),
                                  VectorOffset::Data);
      dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();
      elementType = vectorTypeSym->getType();
      elementMLIRType = getMLIRType(elementType);
    } else {
      auto arraySizeAddr = getArraySizeAddr(*builder, loc, leftMlirType,
                                            ctx->getArrayInstance()->getEvaluatedAddr());
      collectionSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);
      auto dataAddr = getArrayDataAddr(*builder, loc, leftMlirType,
                                       ctx->getArrayInstance()->getEvaluatedAddr());
      dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

      auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftInstanceType);
      elementType = arrayTypeSym->getType();
      elementMLIRType = getMLIRType(elementType);
    }

    auto indexValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(indexType), indexAddr);
    auto normalizedIndex =
        normalizeIndex(indexValue, collectionSize); // converted to 0-indexed form

    // elementPtr = &dataPtr[normalizedIndex]
    auto elementPtrOp = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dataPtr,
                                                           mlir::ValueRange{normalizedIndex});
    mlir::Value elementPtr = elementPtrOp.getResult();
    ctx->setEvaluatedAddr(elementPtr);

  } else if (ctx->getElementIndex()->getNodeType() == ast::NodeType::RangedIndexExpr) {
    std::shared_ptr<symTable::Type> rightIndexType;
    mlir::Value rightIndexAddr;
    bool hasRight = false;
    if (auto rightIndexExpr =
            std::dynamic_pointer_cast<ast::expressions::RangedIndexExprAst>(ctx->getElementIndex())
                ->getRightIndexExpr()) {
      auto [tempRightIndexType, tempRightIndexAddr] = popElementFromStack(rightIndexExpr);
      rightIndexType = tempRightIndexType;
      rightIndexAddr = tempRightIndexAddr;
      hasRight = true;
    }

    auto leftIndexExpr =
        std::dynamic_pointer_cast<ast::expressions::RangedIndexExprAst>(ctx->getElementIndex())
            ->getLeftIndexExpr();
    auto [leftIndexType, leftIndexAddr] = popElementFromStack(leftIndexExpr);

    auto leftVal =
        builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(leftIndexType), leftIndexAddr)
            .getResult();

    mlir::Value dataAddr;
    mlir::Value dataPtr;
    mlir::Value collectionSize;
    if (isVector) {
      dataAddr = gepOpVector(leftMlirType, ctx->getArrayInstance()->getEvaluatedAddr(),
                             VectorOffset::Data);
      dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

      auto vectorSizeAddr = gepOpVector(leftMlirType, ctx->getArrayInstance()->getEvaluatedAddr(),
                                        VectorOffset::Size);
      collectionSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), vectorSizeAddr);
    } else {
      dataAddr = getArrayDataAddr(*builder, loc, leftMlirType,
                                  ctx->getArrayInstance()->getEvaluatedAddr());
      dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr).getResult();

      auto arraySizeAddr = getArraySizeAddr(*builder, loc, leftMlirType,
                                            ctx->getArrayInstance()->getEvaluatedAddr());
      collectionSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);
    }

    mlir::Value normLeft = normalizeIndex(leftVal, collectionSize);

    mlir::Value normRight;
    if (hasRight) {
      auto rightVal =
          builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(rightIndexType), rightIndexAddr)
              .getResult();
      normRight = normalizeIndex(rightVal, collectionSize);
    } else {
      normRight = collectionSize;
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

    // Now create the slice array/vector struct (an alloca for the struct so we can return its
    // address).
    std::shared_ptr<symTable::Type> elementType;
    mlir::Type elementMLIRType;

    if (isVector) {
      auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftInstanceType);
      elementType = vectorTypeSym->getType();
      elementMLIRType = getMLIRType(elementType);

      auto sliceStructAlloca =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), leftMlirType, constOne());

      auto sliceDataAddr = gepOpVector(leftMlirType, sliceStructAlloca, VectorOffset::Data);
      auto sliceSizeAddr = gepOpVector(leftMlirType, sliceStructAlloca, VectorOffset::Size);
      auto sliceCapacityAddr = gepOpVector(leftMlirType, sliceStructAlloca, VectorOffset::Capacity);
      auto sliceIs2DAddr = gepOpVector(leftMlirType, sliceStructAlloca, VectorOffset::Is2D);

      auto srcStartOp = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dataPtr,
                                                           mlir::ValueRange{normLeft});
      mlir::Value srcStart = srcStartOp.getResult();

      builder->create<mlir::LLVM::StoreOp>(loc, srcStart, sliceDataAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, sliceSize, sliceSizeAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, sliceSize, sliceCapacityAddr);

      auto origIs2DAddr = gepOpVector(leftMlirType, ctx->getArrayInstance()->getEvaluatedAddr(),
                                      VectorOffset::Is2D);
      mlir::Value origIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), origIs2DAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, origIs2D, sliceIs2DAddr);

      ctx->setEvaluatedAddr(sliceStructAlloca);
    } else {
      auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(
          ctx->getArrayInstance()->getAssignSymbolType());
      elementType = arrayTypeSym->getType();
      elementMLIRType = getMLIRType(elementType);

      auto sliceStructAlloca =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), leftMlirType, constOne());

      // Pointers to fields inside the new struct
      auto sliceDataAddr = getArrayDataAddr(*builder, loc, leftMlirType, sliceStructAlloca);
      auto sliceSizeAddr = getArraySizeAddr(*builder, loc, leftMlirType, sliceStructAlloca);
      auto sliceIs2DAddr = get2DArrayBoolAddr(*builder, loc, leftMlirType, sliceStructAlloca);

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
      auto origIs2DAddr = get2DArrayBoolAddr(*builder, loc, leftMlirType,
                                             ctx->getArrayInstance()->getEvaluatedAddr());
      mlir::Value origIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), origIs2DAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, origIs2D, sliceIs2DAddr);

      // push the address of this slice struct as the evaluated result
      ctx->setEvaluatedAddr(sliceStructAlloca);
    }
  }

  return {};
}

} // namespace gazprea::backend
