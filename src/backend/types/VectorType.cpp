#include "ast/types/ArrayTypeAst.h"
#include "symTable/VectorTypeSymbol.h"
#include <algorithm>
#include <backend/Backend.h>
#include <string>

namespace gazprea::backend {
std::any Backend::visitVectorType(std::shared_ptr<ast::types::VectorTypeAst> ctx) {
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(ctx->getSymbol());
  if (auto elementArrayType =
          std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(ctx->getElementType())) {
    (void)elementArrayType;
    vectorTypeSym->isScalar = false;
  }
  if (vectorTypeSym->isScalar) {
    // We don't need to evaluate anything for scalar types
    return {};
  }

  vectorTypeSym->declaredElementSize.clear();

  // Either 1D or 2D element type
  const auto arrayType = std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(ctx->getElementType());
  const auto sizeExpr = arrayType->getSizes();
  for (size_t i = 0; i < sizeExpr.size(); ++i) {
    visit(sizeExpr[i]);

    auto [type, valueAddr] = popElementFromStack(sizeExpr[i]);

    // Could be * or an integer value
    mlir::Value recordedSizeAddr = valueAddr;
    if (!type || type->getName() != "integer") {
      int inferredSize = 0;
      if (i < vectorTypeSym->inferredElementSize.size()) {
        inferredSize = vectorTypeSym->inferredElementSize[i];
      }
      auto ifrValue = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), inferredSize);
      recordedSizeAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
      builder->create<mlir::LLVM::StoreOp>(loc, ifrValue, recordedSizeAddr);
    }

    vectorTypeSym->declaredElementSize.push_back(recordedSizeAddr);
  }

  return {};
}

mlir::Value
Backend::createVectorValue(const std::shared_ptr<symTable::VectorTypeSymbol> &vectorType,
                           const std::shared_ptr<symTable::Type> &sourceType,
                           mlir::Value sourceAddr) {
  if (!vectorType || !sourceType)
    return {};

  auto loadSizesFromVector = [&](const std::shared_ptr<symTable::VectorTypeSymbol> &type) {
    std::vector<mlir::Value> sizes;
    if (!type)
      return sizes;
    for (const auto &sizeAddr : type->declaredElementSize) {
      if (!sizeAddr)
        continue;
      sizes.push_back(builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizeAddr));
    }
    return sizes;
  };

  const auto declaredSizes = loadSizesFromVector(vectorType);
  auto throwSizeErrorFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
      "throwVectorSizeError_019addc9_1a57_7674_b3dd_79d0624d2029");

  auto emitSizeValidation = [&](const std::vector<mlir::Value> &actualSizes) {
    if (!throwSizeErrorFunc || declaredSizes.empty() || actualSizes.empty())
      return;

    const auto dims = std::min(declaredSizes.size(), actualSizes.size());
    for (size_t i = 0; i < dims; ++i) {
      auto exceeds = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sgt,
                                                         actualSizes[i], declaredSizes[i]);
      builder->create<mlir::scf::IfOp>(
          loc, exceeds,
          [&](mlir::OpBuilder &b, mlir::Location l) {
            b.create<mlir::LLVM::CallOp>(l, throwSizeErrorFunc, mlir::ValueRange{});
            b.create<mlir::scf::YieldOp>(l);
          },
          [&](mlir::OpBuilder &b, mlir::Location l) { b.create<mlir::scf::YieldOp>(l); });
    }
  };

  auto buildInferredSizeConstants = [&]() {
    std::vector<mlir::Value> inferredSizes;
    for (int inferred : vectorType->inferredElementSize) {
      inferredSizes.push_back(builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), inferred));
    }
    return inferredSizes;
  };

  auto vectorStructTy = getMLIRType(vectorType);
  auto vectorStructAddr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), vectorStructTy, constOne());

  auto makeIndexConst = [&](int idx) {
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
  };

  auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
    return builder->create<mlir::LLVM::GEPOp>(
        loc, ptrTy(), structTy, baseAddr,
        mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
  };

  auto storeVectorField = [&](const VectorOffset offset, mlir::Value value) {
    auto destPtr = makeFieldPtr(vectorStructTy, vectorStructAddr, offset);
    builder->create<mlir::LLVM::StoreOp>(loc, value, destPtr);
  };

  const auto sourceName = sourceType->getName();
  if (sourceName.substr(0, 5) == "array") {
    if (!declaredSizes.empty()) {
      emitSizeValidation(buildInferredSizeConstants());
    }

    auto arrayStructTy = getMLIRType(sourceType);
    auto loadField = [&](const VectorOffset offset, mlir::Type fieldType) {
      auto fieldPtr = makeFieldPtr(arrayStructTy, sourceAddr, offset);
      return builder->create<mlir::LLVM::LoadOp>(loc, fieldType, fieldPtr);
    };

    mlir::Value inferredSize = loadField(VectorOffset::Size, intTy());

    auto clonedArrayStruct =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayStructTy, constOne());

    copyArrayStruct(sourceType, sourceAddr, clonedArrayStruct);

    auto ensureVectorElementCapacity = [&](mlir::Value targetInnerSize) {
      if (!targetInnerSize)
        return;
      auto clonedOuterSizeAddr = getArraySizeAddr(*builder, loc, arrayStructTy, clonedArrayStruct);
      auto currentOuterSize =
          builder->create<mlir::LLVM::LoadOp>(loc, intTy(), clonedOuterSizeAddr);
      padArrayIfNeeded(clonedArrayStruct, sourceType, currentOuterSize, targetInnerSize);
    };

    if (!declaredSizes.empty()) {
      ensureVectorElementCapacity(declaredSizes.front());
    } else if (!vectorType->inferredElementSize.empty()) {
      auto inferredInnerSize = builder->create<mlir::LLVM::ConstantOp>(
          loc, intTy(), vectorType->inferredElementSize.front());
      ensureVectorElementCapacity(inferredInnerSize);
    }

    auto clonedDataPtr = builder->create<mlir::LLVM::LoadOp>(
        loc, ptrTy(), getArrayDataAddr(*builder, loc, arrayStructTy, clonedArrayStruct));
    auto clonedIs2dValue = builder->create<mlir::LLVM::LoadOp>(
        loc, boolTy(), get2DArrayBoolAddr(*builder, loc, arrayStructTy, clonedArrayStruct));

    storeVectorField(VectorOffset::Size, inferredSize);
    storeVectorField(VectorOffset::Capacity, inferredSize);
    storeVectorField(VectorOffset::Data, clonedDataPtr);
    storeVectorField(VectorOffset::Is2D, clonedIs2dValue);
    return vectorStructAddr;
  }

  if (sourceName.substr(0, 6) == "vector") {
    if (!declaredSizes.empty()) {
      if (auto sourceVectorType =
              std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(sourceType)) {
        emitSizeValidation(loadSizesFromVector(sourceVectorType));
      }
    }
    copyValue(vectorType, sourceAddr, vectorStructAddr);
    return vectorStructAddr;
  }

  auto zeroSize = constZero();
  auto boolZero = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
  auto nullPtrInt = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);
  auto nullPtr = builder->create<mlir::LLVM::IntToPtrOp>(loc, ptrTy(), nullPtrInt);

  storeVectorField(VectorOffset::Size, zeroSize);
  storeVectorField(VectorOffset::Capacity, zeroSize);
  storeVectorField(VectorOffset::Data, nullPtr);
  storeVectorField(VectorOffset::Is2D, boolZero);
  return vectorStructAddr;
}

void Backend::copyVectorStruct(std::shared_ptr<symTable::Type> type, mlir::Value fromVectorStruct,
                               mlir::Value destVectorStruct) {
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type);
  if (!vectorTypeSym) {
    return;
  }

  auto vectorStructType = getMLIRType(type);
  auto elementType = vectorTypeSym->getType();
  auto elementMLIRType = getMLIRType(elementType);

  auto makeIndexConst = [&](int idx) {
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
  };

  auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
    return builder->create<mlir::LLVM::GEPOp>(
        loc, ptrTy(), structTy, baseAddr,
        mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
  };

  // Copy Size
  auto srcSizePtr = makeFieldPtr(vectorStructType, fromVectorStruct, VectorOffset::Size);
  auto srcSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizePtr);
  auto destSizePtr = makeFieldPtr(vectorStructType, destVectorStruct, VectorOffset::Size);
  builder->create<mlir::LLVM::StoreOp>(loc, srcSize, destSizePtr);

  // Copy Capacity
  auto srcCapacityPtr = makeFieldPtr(vectorStructType, fromVectorStruct, VectorOffset::Capacity);
  auto srcCapacity = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcCapacityPtr);
  auto destCapacityPtr = makeFieldPtr(vectorStructType, destVectorStruct, VectorOffset::Capacity);
  builder->create<mlir::LLVM::StoreOp>(loc, srcCapacity, destCapacityPtr);

  // Copy Is2D
  auto srcIs2DPtr = makeFieldPtr(vectorStructType, fromVectorStruct, VectorOffset::Is2D);
  auto srcIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), srcIs2DPtr);
  auto destIs2DPtr = makeFieldPtr(vectorStructType, destVectorStruct, VectorOffset::Is2D);
  builder->create<mlir::LLVM::StoreOp>(loc, srcIs2D, destIs2DPtr);

  // Deep Copy Data
  auto srcDataPtrField = makeFieldPtr(vectorStructType, fromVectorStruct, VectorOffset::Data);
  auto srcDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), srcDataPtrField);

  // Allocate new memory for destination data (Capacity)
  mlir::Value destDataPtr = mallocArray(elementMLIRType, srcCapacity);
  auto destDataPtrField = makeFieldPtr(vectorStructType, destVectorStruct, VectorOffset::Data);
  builder->create<mlir::LLVM::StoreOp>(loc, destDataPtr, destDataPtrField);

  // Copy elements (Size)
  if (isTypeArray(elementType) || isTypeVector(elementType)) {
    // For nested types, use copyValue (which handles recursion)
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), srcSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, srcDataPtr,
                                                        mlir::ValueRange{i});
          auto destElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, destDataPtr,
                                                         mlir::ValueRange{i});

          copyValue(elementType, srcElemPtr, destElemPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  } else {
    // For scalar elements, simple load/store
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), srcSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, srcDataPtr,
                                                        mlir::ValueRange{i});
          auto destElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, destDataPtr,
                                                         mlir::ValueRange{i});

          auto elementValue = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, srcElemPtr);
          b.create<mlir::LLVM::StoreOp>(l, elementValue, destElemPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  }
}
} // namespace gazprea::backend
