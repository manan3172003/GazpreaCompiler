#include "ast/types/ArrayTypeAst.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/VectorTypeSymbol.h"
#include <algorithm>
#include <backend/Backend.h>
#include <memory>
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
  auto throwSizeErrorFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowVectorSizeErrorName);

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

  auto enforceInferredVectorLimit = [&](mlir::Value arrayStruct,
                                        const std::shared_ptr<symTable::Type> &arrayType) {
    if (!throwSizeErrorFunc || !arrayStruct || !arrayType)
      return;
    if (!vectorType || vectorType->inferredElementSize.empty())
      return;
    if (!declaredSizes.empty())
      return;
    const auto &flags = vectorType->getElementSizeInferenceFlags();
    const bool shouldCheck =
        std::any_of(flags.begin(), flags.end(), [](bool flag) { return flag; });
    if (!shouldCheck)
      return;

    auto inferredLimit = builder->create<mlir::LLVM::ConstantOp>(
        loc, intTy(), vectorType->inferredElementSize.front());
    auto actualMax = maxSubArraySize(arrayStruct, arrayType);
    auto exceeds = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sgt,
                                                       actualMax, inferredLimit);
    builder->create<mlir::scf::IfOp>(
        loc, exceeds,
        [&](mlir::OpBuilder &b, mlir::Location l) {
          b.create<mlir::LLVM::CallOp>(l, throwSizeErrorFunc, mlir::ValueRange{});
          b.create<mlir::scf::YieldOp>(l);
        },
        [&](mlir::OpBuilder &b, mlir::Location l) { b.create<mlir::scf::YieldOp>(l); });
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

    // Load size from array struct using the correct array accessor
    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructTy, sourceAddr);
    mlir::Value inferredSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);

    auto clonedArrayStruct =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayStructTy, constOne());

    copyArrayStruct(sourceType, sourceAddr, clonedArrayStruct);
    enforceInferredVectorLimit(clonedArrayStruct, sourceType);

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

    auto sourceVectorStructTy = getMLIRType(sourceType);
    auto loadVectorField = [&](VectorOffset offset, mlir::Type fieldType) {
      auto fieldPtr = makeFieldPtr(sourceVectorStructTy, sourceAddr, offset);
      return builder->create<mlir::LLVM::LoadOp>(loc, fieldType, fieldPtr);
    };

    auto srcSize = loadVectorField(VectorOffset::Size, intTy());
    auto srcCapacity = loadVectorField(VectorOffset::Capacity, intTy());
    auto srcDataPtr = loadVectorField(VectorOffset::Data, ptrTy());
    auto srcIs2DValue = loadVectorField(VectorOffset::Is2D, boolTy());

    auto elementType = vectorType->getType();
    auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

    if (elementArrayType) {
      auto pseudoArrayType = std::make_shared<symTable::ArrayTypeSymbol>("array");
      pseudoArrayType->setType(elementType);
      auto pseudoArrayStructTy = getMLIRType(pseudoArrayType);

      auto sourceArrayStruct =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), pseudoArrayStructTy, constOne());
      auto clonedArrayStruct =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), pseudoArrayStructTy, constOne());

      auto sizeAddr = getArraySizeAddr(*builder, loc, pseudoArrayStructTy, sourceArrayStruct);
      builder->create<mlir::LLVM::StoreOp>(loc, srcSize, sizeAddr);
      auto dataAddr = getArrayDataAddr(*builder, loc, pseudoArrayStructTy, sourceArrayStruct);
      builder->create<mlir::LLVM::StoreOp>(loc, srcDataPtr, dataAddr);
      auto is2dAddr = get2DArrayBoolAddr(*builder, loc, pseudoArrayStructTy, sourceArrayStruct);
      builder->create<mlir::LLVM::StoreOp>(loc, srcIs2DValue, is2dAddr);

      copyArrayStruct(pseudoArrayType, sourceArrayStruct, clonedArrayStruct);
      enforceInferredVectorLimit(clonedArrayStruct, pseudoArrayType);

      if (!declaredSizes.empty()) {
        padArrayIfNeeded(clonedArrayStruct, pseudoArrayType, srcSize, declaredSizes.front());
      }

      auto clonedSize = builder->create<mlir::LLVM::LoadOp>(
          loc, intTy(), getArraySizeAddr(*builder, loc, pseudoArrayStructTy, clonedArrayStruct));
      auto clonedDataPtr = builder->create<mlir::LLVM::LoadOp>(
          loc, ptrTy(), getArrayDataAddr(*builder, loc, pseudoArrayStructTy, clonedArrayStruct));
      auto clonedIs2D = builder->create<mlir::LLVM::LoadOp>(
          loc, boolTy(), get2DArrayBoolAddr(*builder, loc, pseudoArrayStructTy, clonedArrayStruct));

      storeVectorField(VectorOffset::Size, clonedSize);
      storeVectorField(VectorOffset::Capacity, srcCapacity);
      storeVectorField(VectorOffset::Data, clonedDataPtr);
      storeVectorField(VectorOffset::Is2D, clonedIs2D);
      return vectorStructAddr;
    }

    auto elementMLIRType = getMLIRType(elementType);
    auto newDataPtr = mallocArray(elementMLIRType, srcSize);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), srcSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, srcDataPtr,
                                                           mlir::ValueRange{i});
          auto destElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                            mlir::ValueRange{i});
          auto elementValue = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, srcElementPtr);
          b.create<mlir::LLVM::StoreOp>(l, elementValue, destElementPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    storeVectorField(VectorOffset::Size, srcSize);
    storeVectorField(VectorOffset::Capacity, srcCapacity);
    storeVectorField(VectorOffset::Data, newDataPtr);
    storeVectorField(VectorOffset::Is2D, srcIs2DValue);
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

void Backend::fillVectorWithScalarValueWithVectorStruct(
    mlir::Value vectorValueAddr, mlir::Value scalarValue, mlir::Value referenceVectorStruct,
    std::shared_ptr<symTable::Type> vectorType) {
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(vectorType);
  auto elementType = vectorTypeSym->getType();
  auto vectorStructType = getMLIRType(vectorType);

  auto makeIndexConst = [&](int idx) {
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
  };

  auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
    return builder->create<mlir::LLVM::GEPOp>(
        loc, ptrTy(), structTy, baseAddr,
        mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
  };

  // Load size from reference vector
  auto refVectorSizeAddr =
      makeFieldPtr(vectorStructType, referenceVectorStruct, VectorOffset::Size);
  mlir::Value vectorSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), refVectorSizeAddr);

  // Load is2D from reference vector
  auto refVectorIs2DAddr =
      makeFieldPtr(vectorStructType, referenceVectorStruct, VectorOffset::Is2D);
  mlir::Value is2DValue = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), refVectorIs2DAddr);

  if (auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
    // Handle vector of arrays (2D case)
    auto innerElementType = elementArrayType->getType();
    auto innerElementMLIRType = getMLIRType(innerElementType);
    auto subArrayStructType = getMLIRType(elementArrayType);

    auto refVectorDataAddr =
        makeFieldPtr(vectorStructType, referenceVectorStruct, VectorOffset::Data);
    mlir::Value refDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), refVectorDataAddr);
    mlir::Value outerDataPtr = mallocArray(subArrayStructType, vectorSize);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), vectorSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                         outerDataPtr, mlir::ValueRange{i});
          auto refSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                            refDataPtr, mlir::ValueRange{i});

          auto refSubArraySizeAddr = getArraySizeAddr(b, l, subArrayStructType, refSubArrayPtr);
          mlir::Value subArraySize = b.create<mlir::LLVM::LoadOp>(l, intTy(), refSubArraySizeAddr);

          mlir::Value innerDataPtr = mallocArray(innerElementMLIRType, subArraySize);

          b.create<mlir::scf::ForOp>(l, constZero(), subArraySize, constOne(), mlir::ValueRange{},
                                     [&](mlir::OpBuilder &b2, mlir::Location l2, mlir::Value j,
                                         mlir::ValueRange iterArgs2) {
                                       auto elementPtr = b2.create<mlir::LLVM::GEPOp>(
                                           l2, ptrTy(), innerElementMLIRType, innerDataPtr,
                                           mlir::ValueRange{j});
                                       b2.create<mlir::LLVM::StoreOp>(l2, scalarValue, elementPtr);
                                       b2.create<mlir::scf::YieldOp>(l2, mlir::ValueRange{});
                                     });

          auto subArrayDataAddr = getArrayDataAddr(b, l, subArrayStructType, subArrayPtr);
          b.create<mlir::LLVM::StoreOp>(l, innerDataPtr, subArrayDataAddr);
          auto subArraySizeAddr = getArraySizeAddr(b, l, subArrayStructType, subArrayPtr);
          b.create<mlir::LLVM::StoreOp>(l, subArraySize, subArraySizeAddr);
          auto subIs2DFieldPtr = get2DArrayBoolAddr(b, l, subArrayStructType, subArrayPtr);
          auto boolFalse = b.create<mlir::LLVM::ConstantOp>(l, boolTy(), 0);
          b.create<mlir::LLVM::StoreOp>(l, boolFalse, subIs2DFieldPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    auto vectorDataAddr = makeFieldPtr(vectorStructType, vectorValueAddr, VectorOffset::Data);
    builder->create<mlir::LLVM::StoreOp>(loc, outerDataPtr, vectorDataAddr);
    auto vectorSizeAddr = makeFieldPtr(vectorStructType, vectorValueAddr, VectorOffset::Size);
    builder->create<mlir::LLVM::StoreOp>(loc, vectorSize, vectorSizeAddr);
    auto vectorCapacityAddr =
        makeFieldPtr(vectorStructType, vectorValueAddr, VectorOffset::Capacity);
    builder->create<mlir::LLVM::StoreOp>(loc, vectorSize, vectorCapacityAddr);
    auto vectorIs2DAddr = makeFieldPtr(vectorStructType, vectorValueAddr, VectorOffset::Is2D);
    builder->create<mlir::LLVM::StoreOp>(loc, is2DValue, vectorIs2DAddr);
  } else {
    // Handle vector of scalars (1D case)
    auto elementMLIRType = getMLIRType(elementType);
    mlir::Value vectorDataPtr = mallocArray(elementMLIRType, vectorSize);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), vectorSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, vectorDataPtr,
                                                        mlir::ValueRange{i});
          b.create<mlir::LLVM::StoreOp>(l, scalarValue, elementPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    auto vectorDataAddr = makeFieldPtr(vectorStructType, vectorValueAddr, VectorOffset::Data);
    builder->create<mlir::LLVM::StoreOp>(loc, vectorDataPtr, vectorDataAddr);
    auto vectorSizeAddr = makeFieldPtr(vectorStructType, vectorValueAddr, VectorOffset::Size);
    builder->create<mlir::LLVM::StoreOp>(loc, vectorSize, vectorSizeAddr);
    auto vectorCapacityAddr =
        makeFieldPtr(vectorStructType, vectorValueAddr, VectorOffset::Capacity);
    builder->create<mlir::LLVM::StoreOp>(loc, vectorSize, vectorCapacityAddr);
    auto vectorIs2DAddr = makeFieldPtr(vectorStructType, vectorValueAddr, VectorOffset::Is2D);
    auto boolFalse = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
    builder->create<mlir::LLVM::StoreOp>(loc, boolFalse, vectorIs2DAddr);
  }
}

mlir::Value Backend::maxSubVectorSize(mlir::Value vectorStruct,
                                      std::shared_ptr<symTable::Type> vectorType) {
  mlir::Value maxSize = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);

  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(vectorType);
  auto elementType = vectorTypeSym->getType();

  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);
  if (!elementArrayType) {
    return maxSize;
  }

  auto vectorStructType = getMLIRType(vectorType);

  auto makeIndexConst = [&](int idx) {
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
  };

  auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
    return builder->create<mlir::LLVM::GEPOp>(
        loc, ptrTy(), structTy, baseAddr,
        mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
  };

  auto vectorSizeAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Size);
  mlir::Value vectorSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), vectorSizeAddr);

  auto vectorDataAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Data);
  mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), vectorDataAddr);

  auto subArrayStructType = getMLIRType(elementArrayType);

  auto forOp = builder->create<mlir::scf::ForOp>(
      loc, constZero(), vectorSize, constOne(), mlir::ValueRange{maxSize},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        mlir::Value currentMax = iterArgs[0];

        auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType, dataPtr,
                                                       mlir::ValueRange{i});

        auto subArraySizeAddr = b.create<mlir::LLVM::GEPOp>(
            l, ptrTy(), subArrayStructType, subArrayPtr,
            mlir::ValueRange{b.create<mlir::LLVM::ConstantOp>(l, b.getI32Type(), 0),
                             b.create<mlir::LLVM::ConstantOp>(l, b.getI32Type(), 0)});
        mlir::Value subArraySize = b.create<mlir::LLVM::LoadOp>(l, intTy(), subArraySizeAddr);

        auto cmp = b.create<mlir::LLVM::ICmpOp>(l, mlir::LLVM::ICmpPredicate::sgt, subArraySize,
                                                currentMax);
        auto newMax = b.create<mlir::LLVM::SelectOp>(l, cmp, subArraySize, currentMax);

        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{newMax});
      });

  return forOp.getResult(0);
}

void Backend::fillVectorWithScalar(mlir::Value vectorStruct,
                                   std::shared_ptr<symTable::Type> vectorType,
                                   mlir::Value scalarValue,
                                   std::shared_ptr<symTable::Type> scalarType,
                                   mlir::Value targetOuterSize, mlir::Value targetInnerSize) {
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(vectorType);
  if (!vectorTypeSym) {
    return;
  }

  auto vectorStructType = getMLIRType(vectorType);
  auto elementType = vectorTypeSym->getType();
  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);
  auto scalarMLIRType = getMLIRType(scalarType);

  auto makeIndexConst = [&](int idx) {
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
  };

  auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
    return builder->create<mlir::LLVM::GEPOp>(
        loc, ptrTy(), structTy, baseAddr,
        mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
  };

  if (elementArrayType) {
    // Vector of arrays case (2D or 3D)
    auto innerElementType = elementArrayType->getType();
    auto innerElementMLIRType = getMLIRType(innerElementType);
    auto subArrayStructType = getMLIRType(elementType);

    // Check if we have 3D (array of arrays)
    auto innerArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(innerElementType);

    // Allocate outer vector data
    mlir::Value outerDataPtr = mallocArray(subArrayStructType, targetOuterSize);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), targetOuterSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                         outerDataPtr, mlir::ValueRange{i});

          if (innerArrayType) {
            // 3D case: vector of arrays of arrays
            auto innerInnerElementType = innerArrayType->getType();
            auto innerInnerElementMLIRType = getMLIRType(innerInnerElementType);
            auto innerSubArrayStructType = getMLIRType(innerElementType);

            // Allocate middle array data
            mlir::Value middleDataPtr = mallocArray(innerSubArrayStructType, targetInnerSize);

            b.create<mlir::scf::ForOp>(
                l, constZero(), targetInnerSize, constOne(), mlir::ValueRange{},
                [&](mlir::OpBuilder &b2, mlir::Location l2, mlir::Value j,
                    mlir::ValueRange iterArgs2) {
                  auto innerSubArrayPtr = b2.create<mlir::LLVM::GEPOp>(
                      l2, ptrTy(), innerSubArrayStructType, middleDataPtr, mlir::ValueRange{j});

                  // For 3D, we'd need another dimension, but for now just initialize empty
                  // This would need to be extended based on actual 3D requirements
                  auto innerDataAddr =
                      getArrayDataAddr(b2, l2, innerSubArrayStructType, innerSubArrayPtr);
                  auto nullPtrInt = b2.create<mlir::LLVM::ConstantOp>(l2, intTy(), 0);
                  auto nullPtr = b2.create<mlir::LLVM::IntToPtrOp>(l2, ptrTy(), nullPtrInt);
                  b2.create<mlir::LLVM::StoreOp>(l2, nullPtr, innerDataAddr);

                  auto innerSizeAddr =
                      getArraySizeAddr(b2, l2, innerSubArrayStructType, innerSubArrayPtr);
                  auto zeroSize = b2.create<mlir::LLVM::ConstantOp>(l2, intTy(), 0);
                  b2.create<mlir::LLVM::StoreOp>(l2, zeroSize, innerSizeAddr);

                  auto innerIs2DFieldPtr =
                      get2DArrayBoolAddr(b2, l2, innerSubArrayStructType, innerSubArrayPtr);
                  auto boolFalse = b2.create<mlir::LLVM::ConstantOp>(l2, boolTy(), 0);
                  b2.create<mlir::LLVM::StoreOp>(l2, boolFalse, innerIs2DFieldPtr);

                  b2.create<mlir::scf::YieldOp>(l2, mlir::ValueRange{});
                });

            auto subArrayDataAddr = getArrayDataAddr(b, l, subArrayStructType, subArrayPtr);
            b.create<mlir::LLVM::StoreOp>(l, middleDataPtr, subArrayDataAddr);
            auto subArraySizeAddr = getArraySizeAddr(b, l, subArrayStructType, subArrayPtr);
            b.create<mlir::LLVM::StoreOp>(l, targetInnerSize, subArraySizeAddr);
            auto subIs2DFieldPtr = get2DArrayBoolAddr(b, l, subArrayStructType, subArrayPtr);
            auto boolTrue = b.create<mlir::LLVM::ConstantOp>(l, boolTy(), 1);
            b.create<mlir::LLVM::StoreOp>(l, boolTrue, subIs2DFieldPtr);
          } else {
            // 2D case: vector of 1D arrays
            // Allocate inner array
            mlir::Value innerDataPtr = mallocArray(innerElementMLIRType, targetInnerSize);

            // Fill inner array with scalar
            b.create<mlir::scf::ForOp>(
                l, constZero(), targetInnerSize, constOne(), mlir::ValueRange{},
                [&](mlir::OpBuilder &b2, mlir::Location l2, mlir::Value j,
                    mlir::ValueRange iterArgs2) {
                  auto elementPtr = b2.create<mlir::LLVM::GEPOp>(l2, ptrTy(), scalarMLIRType,
                                                                 innerDataPtr, mlir::ValueRange{j});
                  b2.create<mlir::LLVM::StoreOp>(l2, scalarValue, elementPtr);
                  b2.create<mlir::scf::YieldOp>(l2, mlir::ValueRange{});
                });

            // Store inner data pointer in sub-array struct
            auto subArrayDataAddr = getArrayDataAddr(b, l, subArrayStructType, subArrayPtr);
            b.create<mlir::LLVM::StoreOp>(l, innerDataPtr, subArrayDataAddr);

            // Store inner size in sub-array struct
            auto subArraySizeAddr = getArraySizeAddr(b, l, subArrayStructType, subArrayPtr);
            b.create<mlir::LLVM::StoreOp>(l, targetInnerSize, subArraySizeAddr);

            // Set is2D flag for sub-array
            auto subIs2DFieldPtr = get2DArrayBoolAddr(b, l, subArrayStructType, subArrayPtr);
            auto boolFalse = b.create<mlir::LLVM::ConstantOp>(l, boolTy(), 0);
            b.create<mlir::LLVM::StoreOp>(l, boolFalse, subIs2DFieldPtr);
          }

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    // Store outer data pointer in vector struct
    auto vectorDataAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Data);
    builder->create<mlir::LLVM::StoreOp>(loc, outerDataPtr, vectorDataAddr);

    // Store size in vector struct
    auto vectorSizeAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Size);
    builder->create<mlir::LLVM::StoreOp>(loc, targetOuterSize, vectorSizeAddr);

    // Store capacity in vector struct
    auto vectorCapacityAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Capacity);
    builder->create<mlir::LLVM::StoreOp>(loc, targetOuterSize, vectorCapacityAddr);

    // Set is2D flag for vector
    auto vectorIs2DAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Is2D);
    auto boolTrue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
    builder->create<mlir::LLVM::StoreOp>(loc, boolTrue, vectorIs2DAddr);

  } else {
    // 1D vector case - allocate memory
    auto elementMLIRType = getMLIRType(elementType);
    mlir::Value vectorDataPtr = mallocArray(elementMLIRType, targetOuterSize);

    // Fill vector with scalar
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), targetOuterSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, vectorDataPtr,
                                                        mlir::ValueRange{i});
          b.create<mlir::LLVM::StoreOp>(l, scalarValue, elementPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    // Store data pointer in vector struct
    auto vectorDataAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Data);
    builder->create<mlir::LLVM::StoreOp>(loc, vectorDataPtr, vectorDataAddr);

    // Store size in vector struct
    auto vectorSizeAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Size);
    builder->create<mlir::LLVM::StoreOp>(loc, targetOuterSize, vectorSizeAddr);

    // Store capacity in vector struct
    auto vectorCapacityAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Capacity);
    builder->create<mlir::LLVM::StoreOp>(loc, targetOuterSize, vectorCapacityAddr);

    // Set is2D flag for vector
    auto vectorIs2DAddr = makeFieldPtr(vectorStructType, vectorStruct, VectorOffset::Is2D);
    auto boolFalse = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
    builder->create<mlir::LLVM::StoreOp>(loc, boolFalse, vectorIs2DAddr);
  }
}

mlir::Value Backend::concatVectors(std::shared_ptr<symTable::Type> type,
                                   mlir::Value leftVectorStruct, mlir::Value rightVectorStruct) {
  // Get vector type information
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type);
  if (!vectorTypeSym) {
    return {};
  }
  auto elementType = vectorTypeSym->getType();
  if (!elementType) {
    return {};
  }
  auto elementMLIRType = getMLIRType(elementType);
  auto vectorStructType = getMLIRType(type);

  auto makeIndexConst = [&](int idx) {
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
  };

  auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
    return builder->create<mlir::LLVM::GEPOp>(
        loc, ptrTy(), structTy, baseAddr,
        mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
  };

  // Get sizes
  auto leftVectorSizeAddr = makeFieldPtr(vectorStructType, leftVectorStruct, VectorOffset::Size);
  auto rightVectorSizeAddr = makeFieldPtr(vectorStructType, rightVectorStruct, VectorOffset::Size);
  auto leftVectorSize =
      builder->create<mlir::LLVM::LoadOp>(loc, intTy(), leftVectorSizeAddr).getResult();
  auto rightVectorSize =
      builder->create<mlir::LLVM::LoadOp>(loc, intTy(), rightVectorSizeAddr).getResult();

  // Calculate total size
  auto totalSize =
      builder->create<mlir::LLVM::AddOp>(loc, leftVectorSize, rightVectorSize).getResult();

  // Allocate new vector struct
  auto newVectorStruct =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), vectorStructType, constOne());

  // Allocate memory for the concatenated data
  mlir::Value newDataPtr = mallocArray(elementMLIRType, totalSize);

  // Get data pointers from source vectors
  auto leftDataAddr = makeFieldPtr(vectorStructType, leftVectorStruct, VectorOffset::Data);
  mlir::Value leftDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), leftDataAddr);

  auto rightDataAddr = makeFieldPtr(vectorStructType, rightVectorStruct, VectorOffset::Data);
  mlir::Value rightDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), rightDataAddr);

  // Check if elements are arrays (for nested structures)
  if (auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
    // For nested arrays, we need to copy each sub-array struct

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), leftVectorSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, leftDataPtr,
                                                    mlir::ValueRange{i});
          auto dstPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                    mlir::ValueRange{i});

          copyArrayStruct(elementType, srcPtr, dstPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), rightVectorSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto offset = b.create<mlir::LLVM::AddOp>(l, leftVectorSize, i);
          auto srcPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, rightDataPtr,
                                                    mlir::ValueRange{i});
          auto dstPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                    mlir::ValueRange{offset});

          copyArrayStruct(elementType, srcPtr, dstPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  } else {
    // For scalar elements, simple load/store
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), leftVectorSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, leftDataPtr,
                                                    mlir::ValueRange{i});
          auto dstPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                    mlir::ValueRange{i});

          auto value = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, srcPtr);
          b.create<mlir::LLVM::StoreOp>(l, value, dstPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), rightVectorSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto offset = b.create<mlir::LLVM::AddOp>(l, leftVectorSize, i);
          auto srcPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, rightDataPtr,
                                                    mlir::ValueRange{i});
          auto dstPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                    mlir::ValueRange{offset});

          auto value = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, srcPtr);
          b.create<mlir::LLVM::StoreOp>(l, value, dstPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  }

  // Set the data pointer in the new vector struct
  auto newVectorDataAddr = makeFieldPtr(vectorStructType, newVectorStruct, VectorOffset::Data);
  builder->create<mlir::LLVM::StoreOp>(loc, newDataPtr, newVectorDataAddr);

  // Set the size in the new vector struct
  auto newVectorSizeAddr = makeFieldPtr(vectorStructType, newVectorStruct, VectorOffset::Size);
  builder->create<mlir::LLVM::StoreOp>(loc, totalSize, newVectorSizeAddr);

  // Set the capacity (same as size for concatenated result)
  auto newVectorCapacityAddr =
      makeFieldPtr(vectorStructType, newVectorStruct, VectorOffset::Capacity);
  builder->create<mlir::LLVM::StoreOp>(loc, totalSize, newVectorCapacityAddr);

  // Set the is2D flag
  auto leftIs2DAddr = makeFieldPtr(vectorStructType, leftVectorStruct, VectorOffset::Is2D);
  mlir::Value is2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), leftIs2DAddr);
  auto newIs2DAddr = makeFieldPtr(vectorStructType, newVectorStruct, VectorOffset::Is2D);
  builder->create<mlir::LLVM::StoreOp>(loc, is2D, newIs2DAddr);

  return newVectorStruct;
}
} // namespace gazprea::backend
