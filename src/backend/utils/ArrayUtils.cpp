#include "CompileTimeExceptions.h"
#include "ast/walkers/DefRefWalker.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/VectorTypeSymbol.h"

#include <backend/Backend.h>
#include <llvm/ADT/APFloat.h>

namespace gazprea::backend {
void Backend::printArray(mlir::Value arrayStructAddr, std::shared_ptr<symTable::Type> arrayType) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  auto elementType = arrayTypeSym->getType();

  while (auto nestedArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
    elementType = nestedArrayType->getType();
  }

  int elementTypeCode = 0;
  std::string elementTypeName = elementType->getName();

  if (elementTypeName == "integer") {
    elementTypeCode = 0;
  } else if (elementTypeName == "real") {
    elementTypeCode = 1;
  } else if (elementTypeName == "character") {
    elementTypeCode = 2;
  } else if (elementTypeName == "boolean") {
    elementTypeCode = 3;
  }

  auto printArrayFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kPrintArrayName);
  auto elementTypeConst = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), elementTypeCode);
  mlir::ValueRange args = {arrayStructAddr, elementTypeConst};
  builder->create<mlir::LLVM::CallOp>(loc, printArrayFunc, args);
}

void Backend::computeArraySizeIfArray(std::shared_ptr<ast::Ast> ctx,
                                      std::shared_ptr<symTable::Type> type,
                                      mlir::Value arrayStruct) {
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (isTypeArray(type) && variableSymbol) {
    std::shared_ptr<ast::types::ArrayTypeAst> arrayDataType = nullptr;
    std::shared_ptr<symTable::Type> sourceValueType = nullptr;

    if (auto DecAst = std::dynamic_pointer_cast<ast::statements::DeclarationAst>(ctx)) {
      arrayDataType = std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(DecAst->getType());
      if (DecAst->getExpr()) {
        sourceValueType = DecAst->getExpr()->getInferredSymbolType();
      }
    }
    if (auto ProcedureParamAst = std::dynamic_pointer_cast<ast::prototypes::ProcedureParamAst>(ctx))
      arrayDataType =
          std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(ProcedureParamAst->getParamType());
    if (auto FunctionParamAst = std::dynamic_pointer_cast<ast::prototypes::FunctionParamAst>(ctx))
      arrayDataType =
          std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(FunctionParamAst->getParamType());
    auto arrayType =
        std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(variableSymbol->getType());
    size_t dimIndex = 0;
    if (arrayType->getSizes().empty()) {
      for (auto sizeExpr : arrayDataType->getSizes()) {
        visit(sizeExpr);
        auto [sizeType, sizeAddr] = popElementFromStack(sizeExpr);
        if (auto c = std::dynamic_pointer_cast<ast::expressions::CharLiteralAst>(sizeExpr)) {
          if (c->getValue() == '*') {
            // Infer size from the array struct being assigned
            // Only do this if source value is actually an array (not a scalar)
            if (sourceValueType && isTypeArray(sourceValueType)) {
              if (dimIndex == 0) {
                // First dimension - get the outer array size
                auto currentArraySizeAddr =
                    getArraySizeAddr(*builder, loc, getMLIRType(sourceValueType), arrayStruct);
                auto newSizeAddr =
                    builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
                auto curSize =
                    builder->create<mlir::LLVM::LoadOp>(loc, intTy(), currentArraySizeAddr);
                builder->create<mlir::LLVM::StoreOp>(loc, curSize, newSizeAddr);
                sizeAddr = newSizeAddr;
                arrayType->addSize(sizeAddr);
              } else if (dimIndex == 1) {
                // Second dimension - get max sub-array size
                mlir::Value maxSubSize = maxSubArraySize(arrayStruct, sourceValueType);
                auto maxSubSizeAddr =
                    builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
                builder->create<mlir::LLVM::StoreOp>(loc, maxSubSize, maxSubSizeAddr);
                sizeAddr = maxSubSizeAddr;
                arrayType->addSize(sizeAddr);
              }
            }
            // If source is not an array (it's a scalar), don't try to infer size
            // Don't add to sizes - this will leave sizes empty and castScalarToArrayIfNeeded will
            // throw error
          } else {
            throw SizeError(ctx->getLineNumber(), "Size needs to be an integer");
          }
        } else {
          if (sizeType->getName() != "integer") {
            throw SizeError(ctx->getLineNumber(), "Size needs to be an integer");
          }
          arrayType->addSize(sizeAddr);
        }
        dimIndex++;
      }
    }
    if (arrayDataType->getSizes().empty()) {
      auto currentArraySizeAddr = getArraySizeAddr(*builder, loc, getMLIRType(type), arrayStruct);
      auto newSizeAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
      auto curSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), currentArraySizeAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, curSize, newSizeAddr);
      arrayType->addSize(newSizeAddr);

      if (auto elementArrayType =
              std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType->getType())) {
        mlir::Value maxSubSize = maxSubArraySize(arrayStruct, type);
        auto maxSubSizeAddr =
            builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
        builder->create<mlir::LLVM::StoreOp>(loc, maxSubSize, maxSubSizeAddr);
        arrayType->addSize(maxSubSizeAddr);
      }
    }
  }
}
mlir::Value Backend::castScalarToArray(std::shared_ptr<ast::Ast> ctx, mlir::Value scalarValue,
                                       std::shared_ptr<symTable::Type> scalarType,
                                       std::shared_ptr<symTable::Type> arrayType) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  auto elementType = arrayTypeSym->getType();


  // Allocate new array struct
  auto arrayStructType = getMLIRType(arrayType);
  auto newArrayAddr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayStructType, constOne());

  // Compute array sizes if they're not already available
  // For function parameters/return types, sizes are already in arrayTypeSym
  if (arrayTypeSym->getSizes().empty()) {
    computeArraySizeIfArray(ctx, arrayType, newArrayAddr);
  }

  // Count total dimensions
  size_t totalDimensions = 1;
  auto currentType = elementType;
  while (auto nestedArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(currentType)) {
    totalDimensions++;
    currentType = nestedArrayType->getType();
  }

  // Check if sizes are available
  auto sizes = arrayTypeSym->getSizes();
  if (sizes.empty() || sizes.size() < totalDimensions) {
    auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
        "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
    builder->create<mlir::LLVM::CallOp>(loc, throwFunc, mlir::ValueRange{});
    return newArrayAddr;
  }

  // Check if multi-dimensional array
  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

  if (elementArrayType) {
    // 2D array case
    auto innerElementType = elementArrayType->getType();
    auto innerElementMLIRType = getMLIRType(innerElementType);

    mlir::Value outerSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizes[0]);
    mlir::Value innerSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizes[1]);

    auto subArrayStructType = getMLIRType(elementArrayType);
    mlir::Value outerDataPtr = mallocArray(subArrayStructType, outerSize);

    // Fill outer array with sub-arrays
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), outerSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                         outerDataPtr, mlir::ValueRange{i});

          mlir::Value innerDataPtr = mallocArray(innerElementMLIRType, innerSize);

          // Fill inner array with scalar value
          b.create<mlir::scf::ForOp>(l, constZero(), innerSize, constOne(), mlir::ValueRange{},
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
          b.create<mlir::LLVM::StoreOp>(l, innerSize, subArraySizeAddr);

          auto subIs2DFieldPtr = get2DArrayBoolAddr(b, l, subArrayStructType, subArrayPtr);
          auto boolFalse = b.create<mlir::LLVM::ConstantOp>(l, boolTy(), 0);
          b.create<mlir::LLVM::StoreOp>(l, boolFalse, subIs2DFieldPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    // Store in newArrayAddr
    auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, newArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, outerDataPtr, arrayDataAddr);

    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, newArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, outerSize, arraySizeAddr);

    auto is2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, newArrayAddr);
    auto boolTrue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
    builder->create<mlir::LLVM::StoreOp>(loc, boolTrue, is2DFieldPtr);

  } else {
    // 1D array case
    mlir::Value targetSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizes[0]);

    auto elementMLIRType = getMLIRType(elementType);
    mlir::Value arrayDataPtr = mallocArray(elementMLIRType, targetSize);

    // Fill all elements with the scalar value
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), targetSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, arrayDataPtr,
                                                        mlir::ValueRange{i});
          b.create<mlir::LLVM::StoreOp>(l, scalarValue, elementPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, newArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, arrayDataPtr, arrayDataAddr);

    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, newArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, targetSize, arraySizeAddr);

    auto is2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, newArrayAddr);
    auto boolFalse = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
    builder->create<mlir::LLVM::StoreOp>(loc, boolFalse, is2DFieldPtr);
  }

  // Perform array size validation
  arraySizeValidation(ctx, arrayType, newArrayAddr);

  return newArrayAddr;
}

void Backend::arraySizeValidationForArrayStructs(mlir::Value lhsArrayStruct,
                                                 std::shared_ptr<symTable::Type> lhsType,
                                                 mlir::Value srcArrayStruct,
                                                 std::shared_ptr<symTable::Type> srcType) {
  // Only handle when lhsType is an array type
  if (!isTypeArray(lhsType)) {
    return;
  }

  auto lhsArrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lhsType);
  if (!lhsArrayTypeSym) {
    return;
  }

  // Get the target outer size from the lhsArrayStruct (runtime size)
  auto lhsArrayStructType = getMLIRType(lhsType);
  auto targetOuterSizeAddr = getArraySizeAddr(*builder, loc, lhsArrayStructType, lhsArrayStruct);
  mlir::Value targetOuterSize =
      builder->create<mlir::LLVM::LoadOp>(loc, intTy(), targetOuterSizeAddr);

  // Get the current outer size of srcArrayStruct
  auto srcArrayStructType = getMLIRType(srcType);
  auto srcSizeAddr = getArraySizeAddr(*builder, loc, srcArrayStructType, srcArrayStruct);
  mlir::Value srcSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizeAddr);

  // If targetOuterSize < srcSize -> throw
  auto isOuterTooSmall = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt,
                                                             targetOuterSize, srcSize);
  builder->create<mlir::scf::IfOp>(
      loc, isOuterTooSmall.getResult(), [&](mlir::OpBuilder &b, mlir::Location l) {
        auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
            "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
        b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
        b.create<mlir::scf::YieldOp>(l);
      });

  // Default targetInnerSize = 0 (for 1D)
  mlir::Value targetInnerSize = constZero();

  // Check if lhsType is a 2D array
  auto lhsElementType = lhsArrayTypeSym->getType();
  auto lhsElementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lhsElementType);

  if (lhsElementArrayType) {
    // This is a 2D array - get the max sub-array size from lhsArrayStruct
    mlir::Value lhsMaxSubSize = maxSubArraySize(lhsArrayStruct, lhsType);
    targetInnerSize = lhsMaxSubSize;

    // Compute max sub-array size of the source
    mlir::Value srcMaxSubSize = maxSubArraySize(srcArrayStruct, srcType);

    // if targetInnerSize < srcMaxSubSize -> throw
    auto isInnerTooSmall = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt,
                                                               targetInnerSize, srcMaxSubSize);
    builder->create<mlir::scf::IfOp>(
        loc, isInnerTooSmall.getResult(), [&](mlir::OpBuilder &b, mlir::Location l) {
          auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
              "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
          b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
          b.create<mlir::scf::YieldOp>(l);
        });
  }

  // Finally, pad the source array to the target sizes if needed.
  // padArrayIfNeeded will check current sizes and only pad when necessary.
  padArrayIfNeeded(srcArrayStruct, srcType, targetOuterSize, targetInnerSize);
}

void Backend::arraySizeValidation(std::shared_ptr<ast::Ast> ctx,
                                  std::shared_ptr<symTable::Type> type, mlir::Value valueAddr) {
  auto symbol = ctx->getSymbol();
  if (auto assignAst = std::dynamic_pointer_cast<ast::statements::AssignmentAst>(ctx)) {
    symbol = assignAst->getLVal()->getSymbol();
  }
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(symbol);
  if (variableSymbol && isTypeArray(type)) {
    auto arrayType =
        std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(variableSymbol->getType());
    if (arrayType->getSizes().empty()) {
      auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowArraySizeErrorName);
      builder->create<mlir::LLVM::CallOp>(loc, throwFunc, mlir::ValueRange{});
      return;
    }
    mlir::Value oneDimensionSizeAddr = arrayType->getSizes()[0];
    mlir::Value oneDimensionSize =
        builder->create<mlir::LLVM::LoadOp>(loc, intTy(), oneDimensionSizeAddr);

    // Get the current size of the valueAddr array struct
    auto arrayStructType = getMLIRType(type);
    auto currentSizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, valueAddr);
    mlir::Value currentSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), currentSizeAddr);

    // Check if oneDimensionSize < currentSize - if so, throw error
    auto isSizeTooSmall = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt,
                                                              oneDimensionSize, currentSize);

    builder->create<mlir::scf::IfOp>(
        loc, isSizeTooSmall, [&](mlir::OpBuilder &b, mlir::Location l) {
          auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowArraySizeErrorName);
          b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
          b.create<mlir::scf::YieldOp>(l);
        });

    // Check two-dimensional size if applicable
    mlir::Value twoDimentionSize = constZero();
    if (arrayType->getSizes().size() == 2) {
      twoDimentionSize =
          builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arrayType->getSizes()[1]);

      // Get max sub-array size from current array
      mlir::Value maxSubSize = maxSubArraySize(valueAddr, type);

      // Check if twoDimensionSize < maxSubSize - if so, throw error
      auto isTwoDimSizeTooSmall = builder->create<mlir::LLVM::ICmpOp>(
          loc, mlir::LLVM::ICmpPredicate::slt, twoDimentionSize, maxSubSize);

      builder->create<mlir::scf::IfOp>(
          loc, isTwoDimSizeTooSmall, [&](mlir::OpBuilder &b, mlir::Location l) {
            auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowArraySizeErrorName);
            b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
            b.create<mlir::scf::YieldOp>(l);
          });
    }

    padArrayIfNeeded(valueAddr, type, oneDimensionSize, twoDimentionSize);
  }
}

mlir::Value Backend::getArraySizeAddr(mlir::OpBuilder &b, mlir::Location l,
                                      mlir::Type arrayStructType, mlir::Value arrayStruct) const {
  return b.create<mlir::LLVM::GEPOp>(
      l, ptrTy(), arrayStructType, arrayStruct,
      mlir::ValueRange{b.create<mlir::LLVM::ConstantOp>(l, b.getI32Type(), 0),
                       b.create<mlir::LLVM::ConstantOp>(l, b.getI32Type(), 0)});
}

mlir::Value Backend::getArrayDataAddr(mlir::OpBuilder &b, mlir::Location l,
                                      mlir::Type arrayStructType, mlir::Value arrayStruct) const {
  return b.create<mlir::LLVM::GEPOp>(
      l, ptrTy(), arrayStructType, arrayStruct,
      mlir::ValueRange{b.create<mlir::LLVM::ConstantOp>(l, b.getI32Type(), 0),
                       b.create<mlir::LLVM::ConstantOp>(l, b.getI32Type(), 1)});
}

mlir::Value Backend::gepOpVector(mlir::Type vectorStructType, mlir::Value vectorStruct,
                                 VectorOffset offset) const {
  return builder->create<mlir::LLVM::GEPOp>(
      loc, ptrTy(), vectorStructType, vectorStruct,
      mlir::ValueRange{builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
                       builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(),
                                                               static_cast<int>(offset))});
}

mlir::Value Backend::get2DArrayBoolAddr(mlir::OpBuilder &b, mlir::Location l,
                                        mlir::Type arrayStructType, mlir::Value arrayStruct) const {
  return b.create<mlir::LLVM::GEPOp>(
      l, ptrTy(), arrayStructType, arrayStruct,
      mlir::ValueRange{b.create<mlir::LLVM::ConstantOp>(l, b.getI32Type(), 0),
                       b.create<mlir::LLVM::ConstantOp>(l, b.getI32Type(), 2)});
}

mlir::Value Backend::maxSubArraySize(mlir::Value arrayStruct,
                                     std::shared_ptr<symTable::Type> arrayType) {
  mlir::Value maxSize = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);

  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  auto elementType = arrayTypeSym->getType();

  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);
  if (!elementArrayType) {
    return maxSize;
  }

  auto arrayStructType = getMLIRType(arrayType);

  auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
  mlir::Value arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);

  auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
  mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), arrayDataAddr);

  auto subArrayStructType = getMLIRType(elementArrayType);

  auto forOp = builder->create<mlir::scf::ForOp>(
      loc, constZero(), arraySize, constOne(), mlir::ValueRange{maxSize},
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

mlir::LLVM::LLVMFuncOp Backend::getOrCreateMallocFunc() {
  auto mallocFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("malloc");
  if (mallocFunc) {
    return mallocFunc;
  }
  auto savedInsertionPoint = builder->saveInsertionPoint();
  builder->setInsertionPointToStart(module.getBody());
  auto i64Type = builder->getI64Type();
  auto mallocFnType = mlir::LLVM::LLVMFunctionType::get(ptrTy(), {i64Type}, /*isVarArg=*/false);
  mallocFunc = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "malloc", mallocFnType);
  builder->restoreInsertionPoint(savedInsertionPoint);
  return mallocFunc;
}

mlir::Value Backend::mallocArray(mlir::Type elementMLIRType, mlir::Value elementCount) {
  auto mallocFunc = getOrCreateMallocFunc();
  auto i64Type = builder->getI64Type();

  auto tempAlloc = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), elementMLIRType, constOne());

  auto oneI64 = builder->create<mlir::LLVM::ConstantOp>(loc, i64Type, 1);
  auto nextElementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, tempAlloc,
                                                           mlir::ValueRange{oneI64});

  auto tempAllocInt = builder->create<mlir::LLVM::PtrToIntOp>(loc, i64Type, tempAlloc);
  auto nextPtrInt = builder->create<mlir::LLVM::PtrToIntOp>(loc, i64Type, nextElementPtr);
  auto elementSizeI64 = builder->create<mlir::LLVM::SubOp>(loc, nextPtrInt, tempAllocInt);

  auto elementCountI64 = builder->create<mlir::LLVM::SExtOp>(loc, i64Type, elementCount);

  auto bytesToAllocate = builder->create<mlir::LLVM::MulOp>(loc, elementCountI64, elementSizeI64);

  return builder->create<mlir::LLVM::CallOp>(loc, mallocFunc, mlir::ValueRange{bytesToAllocate})
      .getResult();
}

mlir::Value Backend::getTypeSizeInBytes(mlir::Type elementType) {
  auto tempAlloc = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), elementType, constOne());
  auto i64Type = builder->getI64Type();
  auto one = builder->create<mlir::LLVM::ConstantOp>(loc, i64Type, 1);
  auto nextPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementType, tempAlloc,
                                                    mlir::ValueRange{one});
  auto baseInt = builder->create<mlir::LLVM::PtrToIntOp>(loc, i64Type, tempAlloc);
  auto nextInt = builder->create<mlir::LLVM::PtrToIntOp>(loc, i64Type, nextPtr);
  auto elementSizeI64 = builder->create<mlir::LLVM::SubOp>(loc, i64Type, nextInt, baseInt);
  return builder->create<mlir::LLVM::TruncOp>(loc, intTy(), elementSizeI64);
}

mlir::Value Backend::getDefaultValue(std::shared_ptr<symTable::Type> type) {
  std::string typeName = type->getName();

  if (typeName == "integer") {
    return builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);
  } else if (typeName == "real") {
    return builder->create<mlir::LLVM::ConstantOp>(loc, floatTy(), llvm::APFloat(0.0f));
  } else if (typeName == "character") {
    return builder->create<mlir::LLVM::ConstantOp>(loc, charTy(), '\0');
  } else if (typeName == "boolean") {
    return builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), false);
  }

  // Should never reach here
  return mlir::Value();
}

void Backend::padArrayWithValue(mlir::Value arrayStruct, std::shared_ptr<symTable::Type> arrayType,
                                mlir::Value currentSize, mlir::Value targetSize,
                                mlir::Value value) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  auto elementType = arrayTypeSym->getType();
  auto elementMLIRType = getMLIRType(elementType);

  auto arrayStructType = getMLIRType(arrayType);

  auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
  mlir::Value oldDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), arrayDataAddr);

  // Allocate new array with target size
  mlir::Value newDataPtr = mallocArray(elementMLIRType, targetSize);

  // Copy existing elements to new array
  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

  if (elementArrayType) {
    // For nested arrays, use copyArrayStruct
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), currentSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto oldElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, oldDataPtr,
                                                           mlir::ValueRange{i});
          auto newElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                           mlir::ValueRange{i});

          copyArrayStruct(elementType, oldElementPtr, newElementPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  } else {
    // For scalar elements, simple load/store
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), currentSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto oldElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, oldDataPtr,
                                                           mlir::ValueRange{i});
          auto newElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                           mlir::ValueRange{i});

          auto elementValue = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, oldElementPtr);
          b.create<mlir::LLVM::StoreOp>(l, elementValue, newElementPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  }

  // Fill padding elements with default value
  builder->create<mlir::scf::ForOp>(
      loc, currentSize, targetSize, constOne(), mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                      mlir::ValueRange{i});

        b.create<mlir::LLVM::StoreOp>(l, value, elementPtr);

        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
      });

  // Free old array
  freeArray(arrayType, arrayStruct);

  // Update array struct with new data pointer and size
  builder->create<mlir::LLVM::StoreOp>(loc, newDataPtr, arrayDataAddr);

  auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, targetSize, arraySizeAddr);
}

void Backend::createEmptySubArray(mlir::Value subArrayPtr,
                                  std::shared_ptr<symTable::Type> subArrayType,
                                  mlir::Value targetSize, mlir::Value defaultValue) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(subArrayType);
  auto elementType = arrayTypeSym->getType();
  auto elementMLIRType = getMLIRType(elementType);
  auto subArrayStructType = getMLIRType(subArrayType);

  mlir::Value subDataPtr = mallocArray(elementMLIRType, targetSize);

  builder->create<mlir::scf::ForOp>(
      loc, constZero(), targetSize, constOne(), mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, subDataPtr,
                                                      mlir::ValueRange{i});
        b.create<mlir::LLVM::StoreOp>(l, defaultValue, elementPtr);
        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
      });

  auto subArraySizeAddr = getArraySizeAddr(*builder, loc, subArrayStructType, subArrayPtr);
  builder->create<mlir::LLVM::StoreOp>(loc, targetSize, subArraySizeAddr);

  auto subArrayDataAddr = getArrayDataAddr(*builder, loc, subArrayStructType, subArrayPtr);
  builder->create<mlir::LLVM::StoreOp>(loc, subDataPtr, subArrayDataAddr);

  auto is2dValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
  auto subIs2dFieldPtr = get2DArrayBoolAddr(*builder, loc, subArrayStructType, subArrayPtr);
  builder->create<mlir::LLVM::StoreOp>(loc, is2dValue, subIs2dFieldPtr);
}

void Backend::padArrayIfNeeded(mlir::Value arrayStruct, std::shared_ptr<symTable::Type> arrayType,
                               mlir::Value targetOuterSize, mlir::Value targetInnerSize) {

  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  if (!arrayTypeSym)
    return;
  auto elementType = arrayTypeSym->getType();

  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

  if (!elementArrayType) {
    mlir::Value defaultValue = getDefaultValue(elementType);
    if (!defaultValue) {
      return;
    }

    auto arrayStructType = getMLIRType(arrayType);

    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
    mlir::Value currentSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);
    auto needsPadding = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt,
                                                            currentSize, targetOuterSize);

    builder->create<mlir::scf::IfOp>(loc, needsPadding, [&](mlir::OpBuilder &b, mlir::Location l) {
      padArrayWithValue(arrayStruct, arrayType, currentSize, targetOuterSize, defaultValue);
      b.create<mlir::scf::YieldOp>(l);
    });

    return;
  }

  auto innermostType = elementArrayType->getType();

  mlir::Value defaultValue = getDefaultValue(innermostType);
  if (!defaultValue) {
    return;
  }

  auto arrayStructType = getMLIRType(arrayType);

  auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
  mlir::Value arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);

  auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
  mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), arrayDataAddr);

  auto subArrayStructType = getMLIRType(elementArrayType);

  builder->create<mlir::scf::ForOp>(
      loc, constZero(), arraySize, constOne(), mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType, dataPtr,
                                                       mlir::ValueRange{i});

        auto subArraySizeAddr = getArraySizeAddr(b, l, subArrayStructType, subArrayPtr);
        mlir::Value subArraySize = b.create<mlir::LLVM::LoadOp>(l, intTy(), subArraySizeAddr);

        auto needsPadding = b.create<mlir::LLVM::ICmpOp>(l, mlir::LLVM::ICmpPredicate::slt,
                                                         subArraySize, targetInnerSize);

        b.create<mlir::scf::IfOp>(l, needsPadding, [&](mlir::OpBuilder &b2, mlir::Location l2) {
          padArrayWithValue(subArrayPtr, elementArrayType, subArraySize, targetInnerSize,
                            defaultValue);
          b2.create<mlir::scf::YieldOp>(l2);
        });

        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
      });

  // Expand outer array if needed
  auto needsMoreSubArrays = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt,
                                                                arraySize, targetOuterSize);

  builder->create<mlir::scf::IfOp>(
      loc, needsMoreSubArrays, [&](mlir::OpBuilder &b, mlir::Location l) {
        // Allocate new larger array
        mlir::Value newDataPtr = mallocArray(subArrayStructType, targetOuterSize);

        // Copy existing sub-arrays
        b.create<mlir::scf::ForOp>(
            l, constZero(), arraySize, constOne(), mlir::ValueRange{},
            [&](mlir::OpBuilder &b2, mlir::Location l2, mlir::Value i, mlir::ValueRange iterArgs) {
              auto oldSubArrayPtr = b2.create<mlir::LLVM::GEPOp>(l2, ptrTy(), subArrayStructType,
                                                                 dataPtr, mlir::ValueRange{i});
              auto newSubArrayPtr = b2.create<mlir::LLVM::GEPOp>(l2, ptrTy(), subArrayStructType,
                                                                 newDataPtr, mlir::ValueRange{i});

              copyArrayStruct(elementArrayType, oldSubArrayPtr, newSubArrayPtr);

              b2.create<mlir::scf::YieldOp>(l2, mlir::ValueRange{});
            });

        // Create new empty sub-arrays for the added elements
        b.create<mlir::scf::ForOp>(
            l, arraySize, targetOuterSize, constOne(), mlir::ValueRange{},
            [&](mlir::OpBuilder &b2, mlir::Location l2, mlir::Value i, mlir::ValueRange iterArgs) {
              auto newSubArrayPtr = b2.create<mlir::LLVM::GEPOp>(l2, ptrTy(), subArrayStructType,
                                                                 newDataPtr, mlir::ValueRange{i});

              createEmptySubArray(newSubArrayPtr, elementArrayType, targetInnerSize, defaultValue);

              b2.create<mlir::scf::YieldOp>(l2, mlir::ValueRange{});
            });

        // Free old outer array (but not the sub-arrays, they've been copied)
        auto freeFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("free");
        b.create<mlir::LLVM::CallOp>(l, freeFunc, mlir::ValueRange{dataPtr});

        // Update array struct
        b.create<mlir::LLVM::StoreOp>(l, newDataPtr, arrayDataAddr);
        b.create<mlir::LLVM::StoreOp>(l, targetOuterSize, arraySizeAddr);

        b.create<mlir::scf::YieldOp>(l);
      });
}

void Backend::cloneArrayStructure(std::shared_ptr<symTable::Type> type,
                                  mlir::Value sourceArrayStruct, mlir::Value destArrayStruct) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type);
  if (!arrayTypeSym) {
    return;
  }

  auto arrayStructType = getMLIRType(type);
  auto elementType = arrayTypeSym->getType();
  auto elementMLIRType = getMLIRType(elementType);

  auto srcSizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, sourceArrayStruct);
  mlir::Value srcSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizeAddr);

  auto destSizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, srcSize, destSizeAddr);

  mlir::Value destDataPtr = mallocArray(elementMLIRType, srcSize);
  auto destDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, destDataPtr, destDataAddr);

  auto srcIs2dAddr = get2DArrayBoolAddr(*builder, loc, arrayStructType, sourceArrayStruct);
  mlir::Value srcIs2d = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), srcIs2dAddr);
  auto destIs2dAddr = get2DArrayBoolAddr(*builder, loc, arrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, srcIs2d, destIs2dAddr);

  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);
  if (!elementArrayType) {
    return;
  }

  auto sourceDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, sourceArrayStruct);
  mlir::Value sourceDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), sourceDataAddr);

  auto subArrayStructType = getMLIRType(elementType);

  builder->create<mlir::scf::ForOp>(
      loc, constZero(), srcSize, constOne(), mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        auto srcSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                          sourceDataPtr, mlir::ValueRange{i});
        auto destSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                           destDataPtr, mlir::ValueRange{i});
        cloneArrayStructure(elementType, srcSubArrayPtr, destSubArrayPtr);
        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
      });
}

mlir::Value Backend::copyArray(std::shared_ptr<symTable::Type> elementType, mlir::Value srcDataPtr,
                               mlir::Value size) {
  auto elementMLIRType = getMLIRType(elementType);

  // Allocate new memory for destination
  mlir::Value destDataPtr = mallocArray(elementMLIRType, size);

  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

  if (elementArrayType) {
    // Handle nested arrays - need to recursively copy sub-arrays
    auto subArrayStructType = getMLIRType(elementType);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), size, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                            srcDataPtr, mlir::ValueRange{i});

          auto destSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                             destDataPtr, mlir::ValueRange{i});

          copyArrayStruct(elementType, srcSubArrayPtr, destSubArrayPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  } else {
    // Handle scalar elements - simple load/store
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), size, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, srcDataPtr,
                                                           mlir::ValueRange{i});

          auto destElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType,
                                                            destDataPtr, mlir::ValueRange{i});

          mlir::Value element = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, srcElementPtr);
          b.create<mlir::LLVM::StoreOp>(l, element, destElementPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  }

  return destDataPtr;
}

void Backend::copyArrayStruct(std::shared_ptr<symTable::Type> type, mlir::Value fromArrayStruct,
                              mlir::Value destArrayStruct) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type);
  if (!arrayTypeSym) {
    return;
  }

  auto elementType = arrayTypeSym->getType();
  auto arrayStructType = getMLIRType(type);

  // Load source size
  auto srcSizeFieldPtr = getArraySizeAddr(*builder, loc, arrayStructType, fromArrayStruct);
  mlir::Value srcSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizeFieldPtr);

  // Load source data pointer
  auto srcDataFieldPtr = getArrayDataAddr(*builder, loc, arrayStructType, fromArrayStruct);
  mlir::Value srcDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), srcDataFieldPtr);

  // Use copyArray to allocate and copy data
  mlir::Value destDataPtr = copyArray(elementType, srcDataPtr, srcSize);

  // Store the new data pointer in the destination array struct
  auto destDataFieldPtr = getArrayDataAddr(*builder, loc, arrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, destDataPtr, destDataFieldPtr);

  // Copy size field to destination
  auto destSizeFieldPtr = getArraySizeAddr(*builder, loc, arrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, srcSize, destSizeFieldPtr);

  // Copy is2D field to destination
  auto srcIs2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, fromArrayStruct);
  mlir::Value srcIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), srcIs2DFieldPtr);
  auto destIs2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, srcIs2D, destIs2DFieldPtr);
}

void Backend::freeArray(std::shared_ptr<symTable::Type> type, mlir::Value arrayStruct) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type);
  if (!arrayTypeSym) {
    return;
  }

  auto elementType = arrayTypeSym->getType();
  auto arrayStructType = getMLIRType(type);

  auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
  mlir::Value arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);

  auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
  mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), arrayDataAddr);

  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

  if (elementArrayType) {
    auto subArrayStructType = getMLIRType(elementType);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), arraySize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType, dataPtr,
                                                         mlir::ValueRange{i});

          freeArray(elementType, subArrayPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  }

  auto freeFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("free");
  if (!freeFunc) {
    auto savedInsertionPoint = builder->saveInsertionPoint();

    builder->setInsertionPointToStart(module.getBody());

    auto voidType = mlir::LLVM::LLVMVoidType::get(builder->getContext());
    auto freeFnType = mlir::LLVM::LLVMFunctionType::get(voidType, {ptrTy()},
                                                        /*isVarArg=*/false);
    freeFunc = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "free", freeFnType);

    builder->restoreInsertionPoint(savedInsertionPoint);
  }

  builder->create<mlir::LLVM::CallOp>(loc, freeFunc, mlir::ValueRange{dataPtr});

  builder->create<mlir::LLVM::StoreOp>(loc, constZero(), arraySizeAddr);

  auto nullPtr = builder->create<mlir::LLVM::ZeroOp>(loc, ptrTy());
  builder->create<mlir::LLVM::StoreOp>(loc, nullPtr, arrayDataAddr);
}

void Backend::freeVector(std::shared_ptr<symTable::Type> type, mlir::Value vectorStruct) {
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type);
  if (!vectorTypeSym) {
    return;
  }

  auto elementType = vectorTypeSym->getType();
  auto vectorStructType = getMLIRType(type);

  auto vectorSizeAddr = gepOpVector(vectorStructType, vectorStruct, VectorOffset::Size);
  mlir::Value vectorSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), vectorSizeAddr);

  auto vectorDataAddr = gepOpVector(vectorStructType, vectorStruct, VectorOffset::Data);
  mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), vectorDataAddr);

  if (auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
    auto subArrayStructType = getMLIRType(elementType);
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), vectorSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType, dataPtr,
                                                         mlir::ValueRange{i});

          freeArray(elementType, subArrayPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  }

  auto freeFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("free");
  if (!freeFunc) {
    auto savedInsertionPoint = builder->saveInsertionPoint();

    builder->setInsertionPointToStart(module.getBody());

    auto voidType = mlir::LLVM::LLVMVoidType::get(builder->getContext());
    auto freeFnType = mlir::LLVM::LLVMFunctionType::get(voidType, {ptrTy()}, /*isVarArg=*/false);
    freeFunc = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "free", freeFnType);

    builder->restoreInsertionPoint(savedInsertionPoint);
  }

  builder->create<mlir::LLVM::CallOp>(loc, freeFunc, mlir::ValueRange{dataPtr});

  builder->create<mlir::LLVM::StoreOp>(loc, constZero(), vectorSizeAddr);

  auto nullPtr = builder->create<mlir::LLVM::ZeroOp>(loc, ptrTy());
  builder->create<mlir::LLVM::StoreOp>(loc, nullPtr, vectorDataAddr);

  auto is2dAddr = get2DArrayBoolAddr(*builder, loc, vectorStructType, vectorStruct);
  auto boolZero = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
  builder->create<mlir::LLVM::StoreOp>(loc, boolZero, is2dAddr);
}

void Backend::createArrayFromVector(
    std::vector<std::shared_ptr<ast::expressions::ExpressionAst>> elements,
    mlir::Type elementMLIRType, mlir::Value dest) {
  for (size_t i = 0; i < elements.size(); i++) {
    visit(elements[i]);
    auto [elemType, elementValueAddr] = popElementFromStack(elements[i]);

    auto index = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), static_cast<int>(i));
    auto elementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dest,
                                                         mlir::ValueRange{index});

    if (isTypeArray(elemType)) {
      copyArrayStruct(elemType, elementValueAddr, elementPtr);
    } else {
      auto elementValue =
          builder->create<mlir::LLVM::LoadOp>(loc, elementMLIRType, elementValueAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, elementValue, elementPtr);
    }
  }
}

// Normalize a potentially-negative 1-indexed index into a positive 1-indexed index.
// - index: i32 (maybe negative or positive, 1-indexed semantics)
// - arraySize: i32 (positive length)
// Returns: i32 normalized index (1..arraySize). Calls throwIndexError() if out of bounds.
mlir::Value Backend::normalizeIndex(mlir::Value index, mlir::Value arraySize) {
  auto i32 = intTy();

  auto zeroOp = builder->create<mlir::LLVM::ConstantOp>(loc, i32, 0);
  auto oneOp = builder->create<mlir::LLVM::ConstantOp>(loc, i32, 1);
  mlir::Value zero = zeroOp.getResult();
  mlir::Value one = oneOp.getResult();

  // isNegative = index < 0  -> i1
  auto isNegativeOp =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, index, zero);
  mlir::Value isNegative = isNegativeOp.getResult();

  // normalizedIfNegative = arraySize + index + 1
  auto sumSizeIndexOp = builder->create<mlir::LLVM::AddOp>(loc, arraySize, index);
  mlir::Value sumSizeIndex = sumSizeIndexOp.getResult();
  auto normalizedIfNegativeOp = builder->create<mlir::LLVM::AddOp>(loc, sumSizeIndex, one);
  mlir::Value normalizedIfNegative = normalizedIfNegativeOp.getResult();

  // normalized = select(isNegative, normalizedIfNegative, index)
  auto normalizedOp =
      builder->create<mlir::LLVM::SelectOp>(loc, isNegative, normalizedIfNegative, index);
  mlir::Value normalized = normalizedOp.getResult();

  // bounds checks: normalized < 1  OR  normalized > arraySize  -> produce i1
  auto lessThanOneOp =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, normalized, one);
  mlir::Value lessThanOne = lessThanOneOp.getResult();

  auto greaterThanSizeOp = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sgt,
                                                               normalized, arraySize);
  mlir::Value greaterThanSize = greaterThanSizeOp.getResult();

  // OR -> outOfBounds (i1)
  auto outOfBoundsOp = builder->create<mlir::LLVM::OrOp>(loc, lessThanOne, greaterThanSize);
  mlir::Value outOfBounds = outOfBoundsOp.getResult();

  auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
      "throwArrayIndexError_019ae3a1_54f9_7452_b095_6faaebe8aa2e");

  // Use the no-result scf::IfOp form (matches other IfOps in your file).
  builder->create<mlir::scf::IfOp>(loc, outOfBounds, [&](mlir::OpBuilder &b, mlir::Location l) {
    b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
    // mirror the other IfOps: yield (no results)
    b.create<mlir::scf::YieldOp>(l);
  });

  // If we reach here, normalized is in range — return the 1-based normalized index.
  auto zeroBased = builder->create<mlir::LLVM::SubOp>(loc, normalized, one).getResult();
  return zeroBased;
}

// Copy `count` elements from srcArrayStruct -> dstDataPtr (element pointer).
// Assumes source and destination element types are identical (no cast needed).
// - srcArrayStruct: address of the source array struct (runtime struct).
// - srcArrayType: symTable::Type for the source (should be ArrayTypeSymbol).
// - dstDataPtr: a loaded pointer (mlir::Value) to the destination element memory.
// - elementType: the element symTable::Type (the runtime element type).
// - count: number of elements to copy.
void Backend::copyArrayElementsToSlice(mlir::Value srcArrayStruct,
                                       std::shared_ptr<symTable::Type> srcArrayType,
                                       mlir::Value dstDataPtr,
                                       std::shared_ptr<symTable::Type> elementType,
                                       mlir::Value count) {
  auto srcArrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(srcArrayType);
  if (!srcArrayTypeSym)
    return;

  auto srcArrayMlirType = getMLIRType(srcArrayType);
  auto srcDataAddr = getArrayDataAddr(*builder, loc, srcArrayMlirType, srcArrayStruct);
  mlir::Value srcDataPtr =
      builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), srcDataAddr).getResult();

  mlir::Type elementMlirType = getMLIRType(elementType);

  auto zero = constZero();
  auto one = constOne();

  // Check if element is an array (nested array case)
  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

  builder->create<mlir::scf::ForOp>(
      loc, zero, count, one, mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value iv, mlir::ValueRange iterArgs) {
        // src element pointer
        auto srcElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMlirType, srcDataPtr,
                                                      mlir::ValueRange{iv});
        // dst element pointer
        auto dstElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMlirType, dstDataPtr,
                                                      mlir::ValueRange{iv});

        if (elementArrayType) {
          // For nested arrays, we need to use copyArrayStruct
          // Save and restore builder context
          auto savedInsertionPoint = builder->saveInsertionPoint();
          builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
          auto savedLoc = loc;
          loc = l;

          copyArrayStruct(elementType, srcElemPtr, dstElemPtr);

          loc = savedLoc;
          builder->restoreInsertionPoint(savedInsertionPoint);
        } else {
          // For scalar elements, simple load/store
          auto value = b.create<mlir::LLVM::LoadOp>(l, elementMlirType, srcElemPtr);
          b.create<mlir::LLVM::StoreOp>(l, value, dstElemPtr);
        }

        b.create<mlir::scf::YieldOp>(l);
      });
}

} // namespace gazprea::backend
