#include "CompileTimeExceptions.h"
#include "ast/expressions/UnaryAst.h"
#include "ast/walkers/DefRefWalker.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/BuiltInTypeSymbol.h"
#include "symTable/StructTypeSymbol.h"
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

mlir::Value Backend::applyUnaryToScalar(ast::expressions::UnaryOpType op,
                                        std::shared_ptr<symTable::Type> type, mlir::OpBuilder &b,
                                        mlir::Location l, mlir::Value value) {
  switch (op) {
  case ast::expressions::UnaryOpType::MINUS: {
    if (type->getName() == "real") {
      return b.create<mlir::LLVM::FNegOp>(l, value);
    }
    auto negConst = b.create<mlir::LLVM::ConstantOp>(l, intTy(), -1);
    return b.create<mlir::LLVM::MulOp>(l, value, negConst);
  }
  case ast::expressions::UnaryOpType::NOT:
    return b.create<mlir::LLVM::XOrOp>(l, value, b.create<mlir::LLVM::ConstantOp>(l, boolTy(), 1));
  case ast::expressions::UnaryOpType::PLUS:
    return value;
  }
  return value;
}

void Backend::applyUnaryToArray(ast::expressions::UnaryOpType op, mlir::Value arrayStruct,
                                std::shared_ptr<symTable::Type> arrayType) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  if (!arrayTypeSym) {
    return;
  }

  auto elementType = arrayTypeSym->getType();
  auto arrayStructType = getMLIRType(arrayType);

  auto sizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
  mlir::Value size = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizeAddr);

  auto dataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
  mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr);

  auto elementMLIRType = getMLIRType(elementType);

  builder->create<mlir::scf::ForOp>(
      loc, constZero(), size, constOne(), mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        auto elementPtr =
            b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, dataPtr, mlir::ValueRange{i});
        if (std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
          auto savedInsertionPoint = builder->saveInsertionPoint();
          auto savedLoc = loc;
          builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
          loc = l;
          applyUnaryToArray(op, elementPtr, elementType);
          loc = savedLoc;
          builder->restoreInsertionPoint(savedInsertionPoint);
        } else {
          auto elementValue = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, elementPtr);
          auto newValue = applyUnaryToScalar(op, elementType, b, l, elementValue);
          b.create<mlir::LLVM::StoreOp>(l, newValue, elementPtr);
        }
        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
      });
}

void Backend::applyUnaryToVector(ast::expressions::UnaryOpType op, mlir::Value vectorStruct,
                                 std::shared_ptr<symTable::Type> vectorType) {
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(vectorType);
  if (!vectorTypeSym) {
    return;
  }

  auto elementType = vectorTypeSym->getType();
  auto vectorStructType = getMLIRType(vectorType);

  auto vectorSizeAddr = gepOpVector(vectorStructType, vectorStruct, VectorOffset::Size);
  mlir::Value vectorSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), vectorSizeAddr);

  auto vectorDataAddr = gepOpVector(vectorStructType, vectorStruct, VectorOffset::Data);
  mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), vectorDataAddr);

  auto elementMLIRType = getMLIRType(elementType);

  builder->create<mlir::scf::ForOp>(
      loc, constZero(), vectorSize, constOne(), mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        auto elementPtr =
            b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, dataPtr, mlir::ValueRange{i});

        if (std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
          auto savedInsertionPoint = builder->saveInsertionPoint();
          auto savedLoc = loc;
          builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
          loc = l;
          applyUnaryToArray(op, elementPtr, elementType);
          loc = savedLoc;
          builder->restoreInsertionPoint(savedInsertionPoint);
        } else {
          auto elementValue = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, elementPtr);
          auto newValue = applyUnaryToScalar(op, elementType, b, l, elementValue);
          b.create<mlir::LLVM::StoreOp>(l, newValue, elementPtr);
        }
        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
      });
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
            if (sourceValueType && isTypeArray(sourceValueType)) {
              if (dimIndex == 0) {
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
                mlir::Value maxSubSize = maxSubArraySize(arrayStruct, sourceValueType);
                auto maxSubSizeAddr =
                    builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
                builder->create<mlir::LLVM::StoreOp>(loc, maxSubSize, maxSubSizeAddr);
                sizeAddr = maxSubSizeAddr;
                arrayType->addSize(sizeAddr);
              }
            }
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

// Responsibility of caller to free arrayStruct
void Backend::fillArrayFromScalar(mlir::Value arrayStruct,
                                  std::shared_ptr<symTable::ArrayTypeSymbol> arrayTypeSym,
                                  mlir::Value scalarValue) {
  auto elementType = arrayTypeSym->getType();
  auto sizes = arrayTypeSym->getSizes();
  auto arrayStructType = getMLIRType(arrayTypeSym);

  if (auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
    // 2D array case
    auto innerElementType = elementArrayType->getType();
    auto innerElementMLIRType = getMLIRType(innerElementType);
    mlir::Value outerSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizes[0]);
    mlir::Value innerSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizes[1]);
    auto subArrayStructType = getMLIRType(elementArrayType);
    mlir::Value outerDataPtr = mallocArray(subArrayStructType, outerSize);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), outerSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                         outerDataPtr, mlir::ValueRange{i});
          mlir::Value innerDataPtr = mallocArray(innerElementMLIRType, innerSize);
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
    auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, outerDataPtr, arrayDataAddr);
    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, outerSize, arraySizeAddr);
    auto is2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayStruct);
    auto boolTrue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
    builder->create<mlir::LLVM::StoreOp>(loc, boolTrue, is2DFieldPtr);
  } else {
    // 1D array case
    mlir::Value targetSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizes[0]);
    auto elementMLIRType = getMLIRType(elementType);
    mlir::Value arrayDataPtr = mallocArray(elementMLIRType, targetSize);
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), targetSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, arrayDataPtr,
                                                        mlir::ValueRange{i});
          b.create<mlir::LLVM::StoreOp>(l, scalarValue, elementPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
    auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, arrayDataPtr, arrayDataAddr);
    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, targetSize, arraySizeAddr);
    auto is2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayStruct);
    auto boolFalse = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
    builder->create<mlir::LLVM::StoreOp>(loc, boolFalse, is2DFieldPtr);
  }
}

void Backend::fillArrayWithScalarValueWithArrayStruct(mlir::Value arrayValueAddr,
                                                      mlir::Value scalarValue,
                                                      mlir::Value referenceArrayStruct,
                                                      std::shared_ptr<symTable::Type> arrayType) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  auto elementType = arrayTypeSym->getType();
  auto arrayStructType = getMLIRType(arrayType);

  auto refArraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, referenceArrayStruct);
  mlir::Value arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), refArraySizeAddr);

  if (auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
    auto innerElementType = elementArrayType->getType();
    auto innerElementMLIRType = getMLIRType(innerElementType);
    auto subArrayStructType = getMLIRType(elementArrayType);

    auto refArrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, referenceArrayStruct);
    mlir::Value refDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), refArrayDataAddr);
    mlir::Value outerDataPtr = mallocArray(subArrayStructType, arraySize);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), arraySize, constOne(), mlir::ValueRange{},
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

    auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayValueAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, outerDataPtr, arrayDataAddr);
    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayValueAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, arraySize, arraySizeAddr);
    auto is2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayValueAddr);
    auto boolTrue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
    builder->create<mlir::LLVM::StoreOp>(loc, boolTrue, is2DFieldPtr);
  } else {
    auto elementMLIRType = getMLIRType(elementType);
    mlir::Value arrayDataPtr = mallocArray(elementMLIRType, arraySize);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), arraySize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, arrayDataPtr,
                                                        mlir::ValueRange{i});
          b.create<mlir::LLVM::StoreOp>(l, scalarValue, elementPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayValueAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, arrayDataPtr, arrayDataAddr);
    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayValueAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, arraySize, arraySizeAddr);
    auto is2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayValueAddr);
    auto boolFalse = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
    builder->create<mlir::LLVM::StoreOp>(loc, boolFalse, is2DFieldPtr);
  }
}

void Backend::throwIfNotEqualArrayStructs(mlir::Value leftStruct, mlir::Value rightStruct,
                                          std::shared_ptr<symTable::Type> arrayType) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  auto elementType = arrayTypeSym->getType();
  auto arrayStructType = getMLIRType(arrayType);

  // Compare sizes
  auto leftStructSizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, leftStruct);
  auto rightStructSizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, rightStruct);
  mlir::Value leftSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), leftStructSizeAddr);
  mlir::Value rightSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), rightStructSizeAddr);

  auto sizesNotEqual =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, leftSize, rightSize);

  builder->create<mlir::scf::IfOp>(loc, sizesNotEqual, [&](mlir::OpBuilder &b, mlir::Location l) {
    auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowArraySizeErrorName);
    b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
    b.create<mlir::scf::YieldOp>(l);
  });

  auto leftStructDimAddr = get2DArrayBoolAddr(*builder, loc, arrayStructType, leftStruct);
  auto rightStructDimAddr = get2DArrayBoolAddr(*builder, loc, arrayStructType, rightStruct);
  mlir::Value leftIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), leftStructDimAddr);
  mlir::Value rightIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), rightStructDimAddr);

  auto dimNotEqual =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, leftIs2D, rightIs2D);

  builder->create<mlir::scf::IfOp>(loc, dimNotEqual, [&](mlir::OpBuilder &b, mlir::Location l) {
    auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowArraySizeErrorName);
    b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
    b.create<mlir::scf::YieldOp>(l);
  });

  // Check if element type is array (meaning this is 2D) - C++ compile-time check
  if (auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
    // 2D array case - recurse on sub-arrays
    auto leftDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, leftStruct);
    mlir::Value leftDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), leftDataAddr);
    auto rightDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, rightStruct);
    mlir::Value rightDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), rightDataAddr);

    auto subArrayStructType = getMLIRType(elementType);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), leftSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto leftSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                             leftDataPtr, mlir::ValueRange{i});
          auto rightSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                              rightDataPtr, mlir::ValueRange{i});

          // Recursively check sub-array dimensions
          throwIfNotEqualArrayStructs(leftSubArrayPtr, rightSubArrayPtr, elementType);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  }
  // For 1D arrays, we only need to check the outer dimension (already done above)
}

mlir::Value Backend::castScalarToArray(std::shared_ptr<ast::Ast> ctx, mlir::Value scalarValue,
                                       std::shared_ptr<symTable::Type> scalarType,
                                       std::shared_ptr<symTable::Type> arrayType) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  auto arrayStructType = getMLIRType(arrayType);
  auto newArrayAddr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayStructType, constOne());

  if (arrayTypeSym->getSizes().empty()) {
    computeArraySizeIfArray(ctx, arrayType, newArrayAddr);
  }

  size_t totalDimensions = 1;
  auto currentType = arrayTypeSym->getType();
  while (auto nestedArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(currentType)) {
    totalDimensions++;
    currentType = nestedArrayType->getType();
  }

  if (arrayTypeSym->getSizes().empty() || arrayTypeSym->getSizes().size() < totalDimensions) {
    auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
        "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
    builder->create<mlir::LLVM::CallOp>(loc, throwFunc, mlir::ValueRange{});
    return newArrayAddr;
  }

  fillArrayFromScalar(newArrayAddr, arrayTypeSym, scalarValue);
  arraySizeValidation(ctx, arrayType, newArrayAddr);
  return newArrayAddr;
}

void Backend::fillArrayWithScalar(mlir::Value arrayStruct,
                                  std::shared_ptr<symTable::Type> arrayType,
                                  mlir::Value scalarValue,
                                  std::shared_ptr<symTable::Type> scalarType,
                                  mlir::Value targetOuterSize, mlir::Value targetInnerSize) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  if (!arrayTypeSym) {
    return;
  }

  auto arrayStructType = getMLIRType(arrayType);
  auto elementType = arrayTypeSym->getType();
  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);
  auto scalarMLIRType = getMLIRType(scalarType);

  if (elementArrayType) {
    // 2D array case - allocate memory for outer and inner arrays
    auto innerElementType = elementArrayType->getType();
    auto innerElementMLIRType = getMLIRType(innerElementType);
    auto subArrayStructType = getMLIRType(elementType);

    // Allocate outer array
    mlir::Value outerDataPtr = mallocArray(subArrayStructType, targetOuterSize);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), targetOuterSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                         outerDataPtr, mlir::ValueRange{i});

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

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    // Store outer data pointer in main array struct
    auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, outerDataPtr, arrayDataAddr);

    // Store outer size in main array struct
    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, targetOuterSize, arraySizeAddr);

    // Set is2D flag for main array
    auto is2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayStruct);
    auto boolTrue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
    builder->create<mlir::LLVM::StoreOp>(loc, boolTrue, is2DFieldPtr);

  } else {
    // 1D array case - allocate memory
    auto elementMLIRType = getMLIRType(elementType);
    mlir::Value arrayDataPtr = mallocArray(elementMLIRType, targetOuterSize);

    // Fill array with scalar
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), targetOuterSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, arrayDataPtr,
                                                        mlir::ValueRange{i});
          b.create<mlir::LLVM::StoreOp>(l, scalarValue, elementPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    // Store data pointer in array struct
    auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, arrayDataPtr, arrayDataAddr);

    // Store size in array struct
    auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, targetOuterSize, arraySizeAddr);

    // Set is2D flag
    auto is2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayStruct);
    auto boolFalse = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
    builder->create<mlir::LLVM::StoreOp>(loc, boolFalse, is2DFieldPtr);
  }
}

std::pair<mlir::Value, bool> Backend::castStructIfNeeded(mlir::Value lhsArrayStruct,
                                                         std::shared_ptr<symTable::Type> lhsType,
                                                         mlir::Value srcValue,
                                                         std::shared_ptr<symTable::Type> srcType) {
  // Check if we need scalar-to-array conversion
  bool needsScalarToArrayCast = false;
  if (lhsType->getName().substr(0, 5) == "array" &&
      (srcType->getName() == "integer" || srcType->getName() == "real" ||
       srcType->getName() == "character" || srcType->getName() == "boolean")) {
    needsScalarToArrayCast = true;
  }

  if (needsScalarToArrayCast) {
    // srcValue is already the loaded scalar value or an address
    // If it's an address, load it
    mlir::Value scalarValue = srcValue;
    if (srcValue.getType() == ptrTy()) {
      scalarValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(srcType), srcValue);
    }

    // Create a new array struct and fill it with the scalar value
    // Allocate new array with target type
    auto newArrayAddr =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(lhsType), constOne());

    // Get target sizes from lhsArrayStruct
    auto lhsArrayStructType = getMLIRType(lhsType);
    auto targetOuterSizeAddr = getArraySizeAddr(*builder, loc, lhsArrayStructType, lhsArrayStruct);
    mlir::Value targetOuterSize =
        builder->create<mlir::LLVM::LoadOp>(loc, intTy(), targetOuterSizeAddr);

    mlir::Value targetInnerSize = constZero();
    auto lhsArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lhsType);
    auto lhsElementType = lhsArrayType->getType();
    auto lhsElementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lhsElementType);

    if (lhsElementArrayType) {
      // 2D array - get max sub-array size from lhsArrayStruct
      targetInnerSize = maxSubArraySize(lhsArrayStruct, lhsType);
    }

    // Fill the new array with the scalar value and set its size to match target
    fillArrayWithScalar(newArrayAddr, lhsType, scalarValue, srcType, targetOuterSize,
                        targetInnerSize);

    return {newArrayAddr, true}; // memory needs to be freed: true
  }

  // Handle array-to-array operations
  if (srcType->getName().substr(0, 5) != "array" || lhsType->getName().substr(0, 5) != "array") {
    return {srcValue, false};
  }

  auto srcArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(srcType);
  auto lhsArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lhsType);

  if (!srcArrayType || !lhsArrayType) {
    return {srcValue, false};
  }

  // srcValue should be an array struct address
  mlir::Value srcArrayStruct = srcValue;

  // Get target outer size from lhsArrayStruct (runtime size)
  auto lhsArrayStructType = getMLIRType(lhsType);
  auto targetOuterSizeAddr = getArraySizeAddr(*builder, loc, lhsArrayStructType, lhsArrayStruct);
  mlir::Value targetOuterSize =
      builder->create<mlir::LLVM::LoadOp>(loc, intTy(), targetOuterSizeAddr);

  // Get current outer size from srcArrayStruct
  auto srcArrayStructType = getMLIRType(srcType);
  auto srcSizeAddr = getArraySizeAddr(*builder, loc, srcArrayStructType, srcArrayStruct);
  mlir::Value srcSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizeAddr);

  // Validation: check if source size is larger than target size
  auto isSizeTooBig = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sgt,
                                                          srcSize, targetOuterSize);
  builder->create<mlir::scf::IfOp>(
      loc, isSizeTooBig.getResult(), [&](mlir::OpBuilder &b, mlir::Location l) {
        auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
            "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
        b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
        b.create<mlir::scf::YieldOp>(l);
      });

  // Default targetInnerSize = 0 (for 1D)
  mlir::Value targetInnerSize = constZero();

  // Check if lhsType is a 2D array
  auto lhsElementType = lhsArrayType->getType();
  auto lhsElementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lhsElementType);

  if (lhsElementArrayType) {
    // This is a 2D array - get the max sub-array size from lhsArrayStruct
    mlir::Value lhsMaxSubSize = maxSubArraySize(lhsArrayStruct, lhsType);
    targetInnerSize = lhsMaxSubSize;

    // Compute max sub-array size of the source
    mlir::Value srcMaxSubSize = maxSubArraySize(srcArrayStruct, srcType);

    // Validation: check if source inner size is larger than target inner size
    auto isInnerSizeTooBig = builder->create<mlir::LLVM::ICmpOp>(
        loc, mlir::LLVM::ICmpPredicate::sgt, srcMaxSubSize, targetInnerSize);
    builder->create<mlir::scf::IfOp>(
        loc, isInnerSizeTooBig.getResult(), [&](mlir::OpBuilder &b, mlir::Location l) {
          auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
              "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
          b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
          b.create<mlir::scf::YieldOp>(l);
        });
  }

  // Pad the source array to target size if needed
  padArrayIfNeeded(srcArrayStruct, srcType, targetOuterSize, targetInnerSize);

  return {srcArrayStruct, false};
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

void Backend::castScalarToArrayIfNeeded(std::shared_ptr<symTable::Type> targetType,
                                        mlir::Value valueAddr,
                                        std::shared_ptr<symTable::Type> sourceType) {
  if (isTypeArray(targetType) && isScalarType(sourceType)) {
    auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(targetType);
    size_t totalDimensions = 1;
    auto currentType = arrayTypeSym->getType();
    while (auto nestedArrayType =
               std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(currentType)) {
      totalDimensions++;
      currentType = nestedArrayType->getType();
    }

    if (arrayTypeSym->getSizes().empty() || arrayTypeSym->getSizes().size() < totalDimensions) {
      auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
          "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
      builder->create<mlir::LLVM::CallOp>(loc, throwFunc, mlir::ValueRange{});
      return;
    }

    auto scalarValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(sourceType), valueAddr);
    fillArrayFromScalar(valueAddr, arrayTypeSym, scalarValue);
  }
}

void Backend::arraySizeValidation(std::shared_ptr<ast::Ast> ctx,
                                  std::shared_ptr<symTable::Type> type, mlir::Value valueAddr) {
  auto symbol = ctx->getSymbol();
  if (auto assignAst = std::dynamic_pointer_cast<ast::statements::AssignmentAst>(ctx)) {
    symbol = assignAst->getLVal()->getSymbol();
  }
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(symbol);

  // Skip validation when the value originated from an explicit cast; casts handle
  // padding/truncation.
  if (variableSymbol) {
    if (auto decl =
            std::dynamic_pointer_cast<ast::statements::DeclarationAst>(variableSymbol->getDef())) {
      if (decl->getExpr() && decl->getExpr()->getNodeType() == ast::NodeType::Cast) {
        return;
      }
    }
  }

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

    auto arrayStructType = getMLIRType(type);
    auto currentSizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, valueAddr);
    mlir::Value currentSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), currentSizeAddr);

    auto isSizeTooSmall = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt,
                                                              oneDimensionSize, currentSize);

    builder->create<mlir::scf::IfOp>(
        loc, isSizeTooSmall, [&](mlir::OpBuilder &b, mlir::Location l) {
          auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowArraySizeErrorName);
          b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
          b.create<mlir::scf::YieldOp>(l);
        });

    mlir::Value twoDimentionSize = constZero();
    if (arrayType->getSizes().size() == 2) {
      twoDimentionSize =
          builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arrayType->getSizes()[1]);

      mlir::Value maxSubSize = maxSubArraySize(valueAddr, type);

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

  mlir::Value newDataPtr = mallocArray(elementMLIRType, targetSize);

  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

  if (elementArrayType) {
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

  builder->create<mlir::scf::ForOp>(
      loc, currentSize, targetSize, constOne(), mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                      mlir::ValueRange{i});

        b.create<mlir::LLVM::StoreOp>(l, value, elementPtr);

        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
      });

  freeArray(arrayType, arrayStruct);

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

  auto needsMoreSubArrays = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt,
                                                                arraySize, targetOuterSize);

  builder->create<mlir::scf::IfOp>(
      loc, needsMoreSubArrays, [&](mlir::OpBuilder &b, mlir::Location l) {
        mlir::Value newDataPtr = mallocArray(subArrayStructType, targetOuterSize);

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

        b.create<mlir::scf::ForOp>(
            l, arraySize, targetOuterSize, constOne(), mlir::ValueRange{},
            [&](mlir::OpBuilder &b2, mlir::Location l2, mlir::Value i, mlir::ValueRange iterArgs) {
              auto newSubArrayPtr = b2.create<mlir::LLVM::GEPOp>(l2, ptrTy(), subArrayStructType,
                                                                 newDataPtr, mlir::ValueRange{i});

              createEmptySubArray(newSubArrayPtr, elementArrayType, targetInnerSize, defaultValue);

              b2.create<mlir::scf::YieldOp>(l2, mlir::ValueRange{});
            });
        // Free the data inside old sub-arrays before freeing the struct array
        b.create<mlir::scf::ForOp>(
            l, constZero(), arraySize, constOne(), mlir::ValueRange{},
            [&](mlir::OpBuilder &b2, mlir::Location l2, mlir::Value i, mlir::ValueRange iterArgs) {
              auto oldSubArrayPtr = b2.create<mlir::LLVM::GEPOp>(l2, ptrTy(), subArrayStructType,
                                                                 dataPtr, mlir::ValueRange{i});

              // Free the data pointer inside this old sub-array struct
              auto oldSubArrayDataAddr =
                  getArrayDataAddr(b2, l2, subArrayStructType, oldSubArrayPtr);
              mlir::Value oldSubDataPtr =
                  b2.create<mlir::LLVM::LoadOp>(l2, ptrTy(), oldSubArrayDataAddr);

              auto freeFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("free");
              b2.create<mlir::LLVM::CallOp>(l2, freeFunc, mlir::ValueRange{oldSubDataPtr});

              b2.create<mlir::scf::YieldOp>(l2, mlir::ValueRange{});
            });

        auto freeFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("free");
        b.create<mlir::LLVM::CallOp>(l, freeFunc, mlir::ValueRange{dataPtr});

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
  if (!elementMLIRType) {
    // Element type is unresolved (e.g., empty array); nothing to copy, return null
    return builder->create<mlir::LLVM::ZeroOp>(loc, ptrTy());
  }

  mlir::Value destDataPtr = mallocArray(elementMLIRType, size);

  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

  if (elementArrayType) {
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

  auto srcSizeFieldPtr = getArraySizeAddr(*builder, loc, arrayStructType, fromArrayStruct);
  mlir::Value srcSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizeFieldPtr);

  auto srcDataFieldPtr = getArrayDataAddr(*builder, loc, arrayStructType, fromArrayStruct);
  mlir::Value srcDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), srcDataFieldPtr);

  mlir::Value destDataPtr = srcDataPtr;
  // Empty arrays may not have a resolved element type; in that case just copy the
  // struct fields without trying to duplicate element storage.
  if (elementType && getMLIRType(elementType)) {
    destDataPtr = copyArray(elementType, srcDataPtr, srcSize);
  } else {
    destDataPtr = builder->create<mlir::LLVM::ZeroOp>(loc, ptrTy());
  }

  auto destDataFieldPtr = getArrayDataAddr(*builder, loc, arrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, destDataPtr, destDataFieldPtr);

  auto destSizeFieldPtr = getArraySizeAddr(*builder, loc, arrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, srcSize, destSizeFieldPtr);

  auto srcIs2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, fromArrayStruct);
  mlir::Value srcIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), srcIs2DFieldPtr);
  auto destIs2DFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, srcIs2D, destIs2DFieldPtr);
}
bool Backend::isTypeTuple(const std::shared_ptr<symTable::Type> &value) {
  return value->getName() == "tuple";
}
bool Backend::isTypeStruct(const std::shared_ptr<symTable::Type> &value) {
  return value->getName() == "struct";
}

// Helper function that frees fields of composite types (tuples/structs)
void Backend::freeCompositeType(const std::vector<std::shared_ptr<symTable::Type>> &resolvedTypes,
                                mlir::Type compositeStructType, mlir::Value compositeStruct) {
  // Iterate through each field
  for (size_t i = 0; i < resolvedTypes.size(); i++) {
    auto fieldType = resolvedTypes[i];

    // Only free heap-allocated types (skip scalars - they're stack-allocated)
    if (!isTypeArray(fieldType) && !isTypeVector(fieldType) && !isTypeTuple(fieldType) &&
        !isTypeStruct(fieldType)) {
      continue;
    }

    // Get pointer to the i-th field using GEP
    auto gepIndices = std::vector<mlir::Value>{
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i)};

    auto fieldPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), compositeStructType,
                                                       compositeStruct, gepIndices);

    // Recursively free the field (handles nested arrays/structs/etc.)
    freeAllocatedMemory(fieldType, fieldPtr);
  }
}

void Backend::freeTuple(std::shared_ptr<symTable::Type> type, mlir::Value tupleStruct) {
  auto tupleTypeSym = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(type);
  if (!tupleTypeSym) {
    return;
  }

  auto tupleStructType = getMLIRType(type);
  auto resolvedTypes = tupleTypeSym->getResolvedTypes();

  freeCompositeType(resolvedTypes, tupleStructType, tupleStruct);
}

void Backend::freeStruct(std::shared_ptr<symTable::Type> type, mlir::Value structStruct) {
  auto structTypeSym = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(type);
  if (!structTypeSym) {
    return;
  }

  auto structStructType = getMLIRType(type);
  auto resolvedTypes = structTypeSym->getResolvedTypes();

  freeCompositeType(resolvedTypes, structStructType, structStruct);
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
    mlir::Type elementMLIRType, mlir::Value dest,
    std::shared_ptr<symTable::Type> targetElementType) {
  for (size_t i = 0; i < elements.size(); i++) {
    visit(elements[i]);
    auto [elemType, elementValueAddr] = popElementFromStack(elements[i]);

    auto index = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), static_cast<int>(i));
    auto elementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, dest,
                                                         mlir::ValueRange{index});

    elementValueAddr = castIfNeeded(elements[i], elementValueAddr, elemType, targetElementType);

    if (isTypeArray(targetElementType)) {
      copyArrayStruct(targetElementType, elementValueAddr, elementPtr);
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
mlir::Value Backend::concatArrays(std::shared_ptr<symTable::Type> type, mlir::Value leftArrayStruct,
                                  mlir::Value rightArrayStruct) {
  // Get array type information
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type);
  auto elementType = arrayTypeSym->getType();
  auto elementMLIRType = getMLIRType(elementType);
  // element type may be unresolved for empty arrays
  bool elementTypeResolved = elementType && elementMLIRType;
  auto arrayStructType = getMLIRType(type);

  // Get sizes
  auto leftArraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, leftArrayStruct);
  auto rightArraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, rightArrayStruct);
  auto leftArraySize =
      builder->create<mlir::LLVM::LoadOp>(loc, intTy(), leftArraySizeAddr).getResult();
  auto rightArraySize =
      builder->create<mlir::LLVM::LoadOp>(loc, intTy(), rightArraySizeAddr).getResult();

  // Calculate total size
  auto totalSize =
      builder->create<mlir::LLVM::AddOp>(loc, leftArraySize, rightArraySize).getResult();

  // Allocate new array struct
  auto newArrayStruct =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayStructType, constOne());

  // Allocate memory for the concatenated data
  mlir::Value newDataPtr = elementTypeResolved ? mallocArray(elementMLIRType, totalSize)
                                               : builder->create<mlir::LLVM::ZeroOp>(loc, ptrTy());

  // Get data pointers from source arrays
  auto leftDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, leftArrayStruct);
  mlir::Value leftDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), leftDataAddr);

  auto rightDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, rightArrayStruct);
  mlir::Value rightDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), rightDataAddr);

  // Check if elements are arrays (for nested arrays)
  if (elementTypeResolved && std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
    // For nested arrays, we need to copy each sub-array struct

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), leftArraySize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, leftDataPtr,
                                                    mlir::ValueRange{i});
          auto dstPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                    mlir::ValueRange{i});

          copyArrayStruct(elementType, srcPtr, dstPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), rightArraySize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto offset = b.create<mlir::LLVM::AddOp>(l, leftArraySize, i);
          auto srcPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, rightDataPtr,
                                                    mlir::ValueRange{i});
          auto dstPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                    mlir::ValueRange{offset});

          copyArrayStruct(elementType, srcPtr, dstPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  } else {
    // For scalar elements, simple load/store
    if (elementTypeResolved) {
      builder->create<mlir::scf::ForOp>(
          loc, constZero(), leftArraySize, constOne(), mlir::ValueRange{},
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
          loc, constZero(), rightArraySize, constOne(), mlir::ValueRange{},
          [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
            auto offset = b.create<mlir::LLVM::AddOp>(l, leftArraySize, i);
            auto srcPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, rightDataPtr,
                                                      mlir::ValueRange{i});
            auto dstPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                      mlir::ValueRange{offset});

            auto value = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, srcPtr);
            b.create<mlir::LLVM::StoreOp>(l, value, dstPtr);

            b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
          });
    }
  }

  // Set the data pointer in the new array struct
  auto newArrayDataAddr = getArrayDataAddr(*builder, loc, arrayStructType, newArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, newDataPtr, newArrayDataAddr);

  // Set the size in the new array struct
  auto newArraySizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, newArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, totalSize, newArraySizeAddr);

  // Set the is2D flag
  auto leftIs2DAddr = get2DArrayBoolAddr(*builder, loc, arrayStructType, leftArrayStruct);
  mlir::Value is2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), leftIs2DAddr);
  auto newIs2DAddr = get2DArrayBoolAddr(*builder, loc, arrayStructType, newArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, is2D, newIs2DAddr);

  return newArrayStruct;
}

mlir::Value Backend::strideArrayByScalar(std::shared_ptr<symTable::Type> type,
                                         mlir::Value arrayStruct, mlir::Value scalarValue) {
  auto newSizeAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, constZero(), newSizeAddr);
  auto arrayType = getMLIRType(type);

  auto arraySizeAddr = getArraySizeAddr(*builder, loc, arrayType, arrayStruct);
  mlir::Value arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);
  auto arrayDataAddr = getArrayDataAddr(*builder, loc, arrayType, arrayStruct);
  mlir::Value dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), arrayDataAddr);
  auto is2dAddr = get2DArrayBoolAddr(*builder, loc, arrayType, arrayStruct);
  mlir::Value is2d = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), is2dAddr);

  // Get element type to check if we're dealing with nested arrays
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type);
  auto elementType = arrayTypeSym->getType();
  auto elementMLIRType = getMLIRType(elementType);
  auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

  // for loop over array and skip by scalarValue to count how many elements we'll have
  builder->create<mlir::scf::ForOp>(
      loc, constZero(), arraySize, scalarValue, mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        auto newSize = b.create<mlir::LLVM::LoadOp>(l, intTy(), newSizeAddr);
        auto incrementedSize = b.create<mlir::LLVM::AddOp>(l, newSize, constOne());
        b.create<mlir::LLVM::StoreOp>(l, incrementedSize, newSizeAddr);
        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
      });
  auto newSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), newSizeAddr);

  auto newArrayStruct = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayType, constOne());

  auto newArraySizeAddr = getArraySizeAddr(*builder, loc, arrayType, newArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, newSize, newArraySizeAddr);

  auto newDataPtr = mallocArray(elementMLIRType, newSize);
  auto newArrayDataAddr = getArrayDataAddr(*builder, loc, arrayType, newArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, newDataPtr, newArrayDataAddr);

  auto is2dAddrNew = get2DArrayBoolAddr(*builder, loc, arrayType, newArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, is2d, is2dAddrNew);

  // Check if element type is an array (multi-dimensional case)
  if (elementArrayType) {
    // For nested arrays, use copyArrayStruct
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), newSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto offset = b.create<mlir::LLVM::MulOp>(l, i, scalarValue);
          auto srcElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, dataPtr,
                                                           mlir::ValueRange{offset});
          auto destElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                            mlir::ValueRange{i});
          copyArrayStruct(elementType, srcElementPtr, destElementPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  } else {
    // For scalar elements, use load/store
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), newSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto offset = b.create<mlir::LLVM::MulOp>(l, i, scalarValue);
          auto srcElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, dataPtr,
                                                           mlir::ValueRange{offset});
          auto destElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                            mlir::ValueRange{i});
          auto elementValue = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, srcElementPtr);
          b.create<mlir::LLVM::StoreOp>(l, elementValue, destElementPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });
  }

  return newArrayStruct;
}

mlir::Value Backend::areArraysEqual(mlir::Value leftArrayStruct, mlir::Value rightArrayStruct,
                                    std::shared_ptr<symTable::Type> arrayType) {
  auto arrayTypeSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType);
  auto elementType = arrayTypeSym->getType();
  auto elementMLIRType = getMLIRType(elementType);
  auto arrayStructType = getMLIRType(arrayType);

  // Create local constants using the passed builder and location
  auto localConstOne = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 1);
  auto localConstZero = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);

  // Get array sizes
  auto leftSizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, leftArrayStruct);
  mlir::Value leftSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), leftSizeAddr);
  auto rightSizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, rightArrayStruct);
  mlir::Value rightSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), rightSizeAddr);

  // Compare sizes
  auto sizesEqual =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, leftSize, rightSize);

  // Create result variable to hold the final boolean result
  auto resultAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), boolTy(), localConstOne);
  builder->create<mlir::LLVM::StoreOp>(loc, sizesEqual, resultAddr);

  // Only check elements if sizes are equal
  builder->create<mlir::scf::IfOp>(
      loc, sizesEqual,
      [&](mlir::OpBuilder &b, mlir::Location l) {
        // Get data pointers
        auto leftDataAddr = getArrayDataAddr(b, l, arrayStructType, leftArrayStruct);
        mlir::Value leftDataPtr = b.create<mlir::LLVM::LoadOp>(l, ptrTy(), leftDataAddr);
        auto rightDataAddr = getArrayDataAddr(b, l, arrayStructType, rightArrayStruct);
        mlir::Value rightDataPtr = b.create<mlir::LLVM::LoadOp>(l, ptrTy(), rightDataAddr);

        // Check if element type is an array (multi-dimensional case)
        auto elementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType);

        // Loop through elements and compare
        b.create<mlir::scf::ForOp>(
            l, constZero(), leftSize, constOne(), mlir::ValueRange{},
            [&](mlir::OpBuilder &forBuilder, mlir::Location forLoc, mlir::Value i,
                mlir::ValueRange iterArgs) {
              auto leftElementPtr = forBuilder.create<mlir::LLVM::GEPOp>(
                  forLoc, ptrTy(), elementMLIRType, leftDataPtr, mlir::ValueRange{i});
              auto rightElementPtr = forBuilder.create<mlir::LLVM::GEPOp>(
                  forLoc, ptrTy(), elementMLIRType, rightDataPtr, mlir::ValueRange{i});

              mlir::Value elementsEqual;
              if (elementArrayType) {
                // Recursively compare sub-arrays
                elementsEqual = areArraysEqual(leftElementPtr, rightElementPtr, elementType);
                elementsEqual =
                    forBuilder.create<mlir::LLVM::LoadOp>(forLoc, boolTy(), elementsEqual);
              } else {
                // Compare scalar elements
                auto leftValue =
                    forBuilder.create<mlir::LLVM::LoadOp>(forLoc, elementMLIRType, leftElementPtr);
                auto rightValue =
                    forBuilder.create<mlir::LLVM::LoadOp>(forLoc, elementMLIRType, rightElementPtr);

                if (elementType->getName() == "real") {
                  elementsEqual = forBuilder.create<mlir::LLVM::FCmpOp>(
                      forLoc, mlir::LLVM::FCmpPredicate::oeq, leftValue, rightValue);
                } else {
                  elementsEqual = forBuilder.create<mlir::LLVM::ICmpOp>(
                      forLoc, mlir::LLVM::ICmpPredicate::eq, leftValue, rightValue);
                }
              }

              // If elements are not equal, set result to false
              auto currentResult =
                  forBuilder.create<mlir::LLVM::LoadOp>(forLoc, boolTy(), resultAddr);
              auto stillEqual =
                  forBuilder.create<mlir::LLVM::AndOp>(forLoc, currentResult, elementsEqual);
              forBuilder.create<mlir::LLVM::StoreOp>(forLoc, stillEqual, resultAddr);

              forBuilder.create<mlir::scf::YieldOp>(forLoc, mlir::ValueRange{});
            });

        b.create<mlir::scf::YieldOp>(l);
      },
      [&](mlir::OpBuilder &b, mlir::Location l) { b.create<mlir::scf::YieldOp>(l); });

  return resultAddr;
}

bool Backend::typeContainsReal(std::shared_ptr<symTable::Type> type) {
  if (!type) {
    return false;
  }

  // Check if the type itself is real
  if (type->getName() == "real") {
    return true;
  }

  // Check if it's an array type - recursively check element type
  if (auto arrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type)) {
    return typeContainsReal(arrayType->getType());
  }

  // Check if it's a vector type - recursively check element type
  if (auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type)) {
    return typeContainsReal(vectorType->getType());
  }

  return false;
}

bool Backend::typeContainsInteger(std::shared_ptr<symTable::Type> type) {
  if (!type) {
    return false;
  }

  // Check if the type itself is integer
  if (type->getName() == "integer") {
    return true;
  }

  // Check if it's an array type - recursively check element type
  if (auto arrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type)) {
    return typeContainsInteger(arrayType->getType());
  }

  // Check if it's a vector type - recursively check element type
  if (auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type)) {
    return typeContainsInteger(vectorType->getType());
  }

  return false;
}

mlir::Value Backend::castIntegerArrayToReal(mlir::Value fromArrayStruct,
                                            std::shared_ptr<symTable::Type> srcType,
                                            bool shouldCast) {
  // Early return if casting is disabled
  if (!shouldCast) {
    return fromArrayStruct;
  }

  // Cast to ArrayTypeSymbol
  auto srcArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(srcType);
  if (!srcArrayType) {
    return fromArrayStruct;
  }

  auto srcElementType = srcArrayType->getType();

  // Check if this array actually contains integers that need casting
  // If it doesn't contain integers, just return the original
  if (!typeContainsInteger(srcType)) {
    return fromArrayStruct;
  }

  // Get MLIR types - destination uses real (float) elements
  auto srcArrayStructType = getMLIRType(srcType);
  // Destination struct is {i32 size, ptr data, i1 is2D}
  auto destArrayStructType = structTy({intTy(), ptrTy(), boolTy()});

  // Allocate a new destination struct
  mlir::Value destArrayStruct =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), destArrayStructType, constOne());

  // Get source array size and data
  auto srcSizeAddr = getArraySizeAddr(*builder, loc, srcArrayStructType, fromArrayStruct);
  mlir::Value srcSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizeAddr);

  auto srcDataAddr = getArrayDataAddr(*builder, loc, srcArrayStructType, fromArrayStruct);
  mlir::Value srcDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), srcDataAddr);

  // Get destination array data address
  auto destDataAddr = getArrayDataAddr(*builder, loc, destArrayStructType, destArrayStruct);

  // Check if elements are arrays (multi-dimensional case)
  auto srcElementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(srcElementType);

  if (srcElementArrayType) {
    // Multi-dimensional array: allocate array of array structures and recursively cast
    mlir::Value newDestDataPtr = mallocArray(destArrayStructType, srcSize);

    // Iterate through sub-arrays and recursively cast each one
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), srcSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcSubArrayStructType = getMLIRType(srcElementType);

          auto srcSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), srcSubArrayStructType,
                                                            srcDataPtr, mlir::ValueRange{i});

          auto destSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), destArrayStructType,
                                                             newDestDataPtr, mlir::ValueRange{i});

          // Recursively cast sub-array and copy the result
          auto savedInsertionPoint = builder->saveInsertionPoint();
          auto savedLoc = loc;
          builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
          loc = l;

          mlir::Value newSubArray =
              castIntegerArrayToReal(srcSubArrayPtr, srcElementType, shouldCast);
          // Copy the struct fields from newSubArray to destSubArrayPtr
          auto newSubSizeAddr = getArraySizeAddr(b, l, destArrayStructType, newSubArray);
          auto newSubSize = b.create<mlir::LLVM::LoadOp>(l, intTy(), newSubSizeAddr);
          auto destSubSizeAddr = getArraySizeAddr(b, l, destArrayStructType, destSubArrayPtr);
          b.create<mlir::LLVM::StoreOp>(l, newSubSize, destSubSizeAddr);

          auto newSubDataAddr = getArrayDataAddr(b, l, destArrayStructType, newSubArray);
          auto newSubData = b.create<mlir::LLVM::LoadOp>(l, ptrTy(), newSubDataAddr);
          auto destSubDataAddr = getArrayDataAddr(b, l, destArrayStructType, destSubArrayPtr);
          b.create<mlir::LLVM::StoreOp>(l, newSubData, destSubDataAddr);

          auto newSubIs2DAddr = get2DArrayBoolAddr(b, l, destArrayStructType, newSubArray);
          auto newSubIs2D = b.create<mlir::LLVM::LoadOp>(l, boolTy(), newSubIs2DAddr);
          auto destSubIs2DAddr = get2DArrayBoolAddr(b, l, destArrayStructType, destSubArrayPtr);
          b.create<mlir::LLVM::StoreOp>(l, newSubIs2D, destSubIs2DAddr);

          loc = savedLoc;
          builder->restoreInsertionPoint(savedInsertionPoint);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    // Store new data pointer in destination
    builder->create<mlir::LLVM::StoreOp>(loc, newDestDataPtr, destDataAddr);
  } else {
    // 1D array: cast scalar elements from integer to real
    mlir::Value newDestDataPtr = mallocArray(floatTy(), srcSize);

    auto srcElementMLIRType = getMLIRType(srcElementType);

    // Iterate through elements and cast each integer to real
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), srcSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          // Get source element pointer and load integer value
          auto srcElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), srcElementMLIRType,
                                                           srcDataPtr, mlir::ValueRange{i});
          mlir::Value intValue = b.create<mlir::LLVM::LoadOp>(l, srcElementMLIRType, srcElementPtr);

          // Cast integer to real using SIToFPOp
          mlir::Value realValue = b.create<mlir::LLVM::SIToFPOp>(l, floatTy(), intValue);

          // Get destination element pointer and store real value
          auto destElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), floatTy(), newDestDataPtr,
                                                            mlir::ValueRange{i});
          b.create<mlir::LLVM::StoreOp>(l, realValue, destElementPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    // Store new data pointer in destination
    builder->create<mlir::LLVM::StoreOp>(loc, newDestDataPtr, destDataAddr);
  }

  // Store size in destination
  auto destSizeAddr = getArraySizeAddr(*builder, loc, destArrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, srcSize, destSizeAddr);

  // Copy the 2D array bool field if it exists
  auto srcIs2DAddr = get2DArrayBoolAddr(*builder, loc, srcArrayStructType, fromArrayStruct);
  mlir::Value srcIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), srcIs2DAddr);
  auto destIs2DAddr = get2DArrayBoolAddr(*builder, loc, destArrayStructType, destArrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, srcIs2D, destIs2DAddr);

  return destArrayStruct;
}

mlir::Value Backend::castIntegerVectorToReal(mlir::Value fromVectorStruct,
                                             std::shared_ptr<symTable::Type> srcType,
                                             bool shouldCast) {
  // Early return if casting is disabled
  if (!shouldCast) {
    return fromVectorStruct;
  }

  // Cast to VectorTypeSymbol
  auto srcVectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(srcType);
  if (!srcVectorType) {
    return fromVectorStruct;
  }

  auto srcElementType = srcVectorType->getType();

  // Check if this vector actually contains integers that need casting
  // If it doesn't contain integers, just return the original
  if (!typeContainsInteger(srcType)) {
    return fromVectorStruct;
  }

  // Get MLIR types - destination uses real (float) elements
  auto srcVectorStructType = getMLIRType(srcType);
  // Destination struct is {i32 size, i32 capacity, ptr data, i1 is2D}
  auto destVectorStructType = structTy({intTy(), intTy(), ptrTy(), boolTy()});

  // Allocate a new destination struct
  mlir::Value destVectorStruct =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), destVectorStructType, constOne());

  // Helper lambda to access vector struct fields
  auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
    auto makeIndexConst = [&](int idx) {
      return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
    };
    return builder->create<mlir::LLVM::GEPOp>(
        loc, ptrTy(), structTy, baseAddr,
        mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
  };

  // Get source vector size and data
  auto srcSizeAddr = makeFieldPtr(srcVectorStructType, fromVectorStruct, VectorOffset::Size);
  mlir::Value srcSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizeAddr);

  auto srcCapacityAddr =
      makeFieldPtr(srcVectorStructType, fromVectorStruct, VectorOffset::Capacity);
  mlir::Value srcCapacity = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcCapacityAddr);

  auto srcDataAddr = makeFieldPtr(srcVectorStructType, fromVectorStruct, VectorOffset::Data);
  mlir::Value srcDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), srcDataAddr);

  // Get destination vector data address
  auto destDataAddr = makeFieldPtr(destVectorStructType, destVectorStruct, VectorOffset::Data);

  // Check if elements are arrays (vector of arrays case)
  auto srcElementArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(srcElementType);

  if (srcElementArrayType) {
    // Vector of arrays: allocate array of array structures and recursively cast
    mlir::Value newDestDataPtr = mallocArray(destVectorStructType, srcSize);

    // Iterate through sub-arrays and recursively cast each one
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), srcSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcSubArrayStructType = getMLIRType(srcElementType);
          auto destArrayStructType = structTy({intTy(), ptrTy(), boolTy()});

          auto srcSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), srcSubArrayStructType,
                                                            srcDataPtr, mlir::ValueRange{i});

          auto destSubArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), destArrayStructType,
                                                             newDestDataPtr, mlir::ValueRange{i});

          // Recursively cast sub-array and copy the result
          auto savedInsertionPoint = builder->saveInsertionPoint();
          auto savedLoc = loc;
          builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
          loc = l;

          mlir::Value newSubArray =
              castIntegerArrayToReal(srcSubArrayPtr, srcElementType, shouldCast);
          // Copy the struct fields from newSubArray to destSubArrayPtr
          auto newSubSizeAddr = getArraySizeAddr(b, l, destArrayStructType, newSubArray);
          auto newSubSize = b.create<mlir::LLVM::LoadOp>(l, intTy(), newSubSizeAddr);
          auto destSubSizeAddr = getArraySizeAddr(b, l, destArrayStructType, destSubArrayPtr);
          b.create<mlir::LLVM::StoreOp>(l, newSubSize, destSubSizeAddr);

          auto newSubDataAddr = getArrayDataAddr(b, l, destArrayStructType, newSubArray);
          auto newSubData = b.create<mlir::LLVM::LoadOp>(l, ptrTy(), newSubDataAddr);
          auto destSubDataAddr = getArrayDataAddr(b, l, destArrayStructType, destSubArrayPtr);
          b.create<mlir::LLVM::StoreOp>(l, newSubData, destSubDataAddr);

          auto newSubIs2DAddr = get2DArrayBoolAddr(b, l, destArrayStructType, newSubArray);
          auto newSubIs2D = b.create<mlir::LLVM::LoadOp>(l, boolTy(), newSubIs2DAddr);
          auto destSubIs2DAddr = get2DArrayBoolAddr(b, l, destArrayStructType, destSubArrayPtr);
          b.create<mlir::LLVM::StoreOp>(l, newSubIs2D, destSubIs2DAddr);

          loc = savedLoc;
          builder->restoreInsertionPoint(savedInsertionPoint);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    // Store new data pointer in destination
    builder->create<mlir::LLVM::StoreOp>(loc, newDestDataPtr, destDataAddr);
  } else {
    // Vector of scalars: cast scalar elements from integer to real
    mlir::Value newDestDataPtr = mallocArray(floatTy(), srcCapacity);

    auto srcElementMLIRType = getMLIRType(srcElementType);

    // Iterate through elements and cast each integer to real
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), srcSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          // Get source element pointer and load integer value
          auto srcElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), srcElementMLIRType,
                                                           srcDataPtr, mlir::ValueRange{i});
          mlir::Value intValue = b.create<mlir::LLVM::LoadOp>(l, srcElementMLIRType, srcElementPtr);

          // Cast integer to real using SIToFPOp
          mlir::Value realValue = b.create<mlir::LLVM::SIToFPOp>(l, floatTy(), intValue);

          // Get destination element pointer and store real value
          auto destElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), floatTy(), newDestDataPtr,
                                                            mlir::ValueRange{i});
          b.create<mlir::LLVM::StoreOp>(l, realValue, destElementPtr);

          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    // Store new data pointer in destination
    builder->create<mlir::LLVM::StoreOp>(loc, newDestDataPtr, destDataAddr);
  }

  // Store size in destination
  auto destSizeAddr = makeFieldPtr(destVectorStructType, destVectorStruct, VectorOffset::Size);
  builder->create<mlir::LLVM::StoreOp>(loc, srcSize, destSizeAddr);

  // Store capacity in destination
  auto destCapacityAddr =
      makeFieldPtr(destVectorStructType, destVectorStruct, VectorOffset::Capacity);
  builder->create<mlir::LLVM::StoreOp>(loc, srcCapacity, destCapacityAddr);

  // Copy the 2D vector bool field if it exists
  auto srcIs2DAddr = makeFieldPtr(srcVectorStructType, fromVectorStruct, VectorOffset::Is2D);
  mlir::Value srcIs2D = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), srcIs2DAddr);
  auto destIs2DAddr = makeFieldPtr(destVectorStructType, destVectorStruct, VectorOffset::Is2D);
  builder->create<mlir::LLVM::StoreOp>(loc, srcIs2D, destIs2DAddr);

  return destVectorStruct;
}

std::shared_ptr<symTable::Type>
Backend::convertIntegerTypeToRealType(std::shared_ptr<symTable::Type> type) {
  if (!type) {
    return type;
  }

  // Check if it's a basic integer type
  if (type->getName() == "integer") {
    return std::make_shared<symTable::BuiltInTypeSymbol>("real");
  }

  // Check if it's an array type - recursively convert element type
  if (auto arrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type)) {
    auto elementType = arrayType->getType();
    auto convertedElementType = convertIntegerTypeToRealType(elementType);

    // If the element type didn't change, return the original type
    if (convertedElementType == elementType) {
      return type;
    }

    // Create a new array type with the converted element type
    auto newArrayType = std::make_shared<symTable::ArrayTypeSymbol>(arrayType->getName());
    newArrayType->setType(convertedElementType);

    // Copy over sizes and other metadata
    for (auto size : arrayType->getSizes()) {
      newArrayType->addSize(size);
    }
    newArrayType->inferredSize = arrayType->inferredSize;
    newArrayType->inferredElementSize = arrayType->inferredElementSize;
    newArrayType->declaredElementSize = arrayType->declaredElementSize;
    newArrayType->elementSizeInferenceFlags = arrayType->elementSizeInferenceFlags;
    newArrayType->isElement2D = arrayType->isElement2D;

    return newArrayType;
  }

  // Check if it's a vector type - recursively convert element type
  if (auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type)) {
    auto elementType = vectorType->getType();
    auto convertedElementType = convertIntegerTypeToRealType(elementType);

    // If the element type didn't change, return the original type
    if (convertedElementType == elementType) {
      return type;
    }

    // Create a new vector type with the converted element type
    auto newVectorType = std::make_shared<symTable::VectorTypeSymbol>(vectorType->getName());
    newVectorType->setType(convertedElementType);

    // Copy over metadata
    newVectorType->inferredSize = vectorType->inferredSize;
    newVectorType->inferredElementSize = vectorType->inferredElementSize;
    newVectorType->declaredElementSize = vectorType->declaredElementSize;
    newVectorType->setElementSizeInferenceFlags(vectorType->getElementSizeInferenceFlags());
    newVectorType->isScalar = vectorType->isScalar;
    newVectorType->isElement2D = vectorType->isElement2D;

    return newVectorType;
  }

  // For all other types, return unchanged
  return type;
}

bool Backend::isTypeReal(std::shared_ptr<symTable::Type> type) {
  return type && type->getName().find("real") != std::string::npos;
}

bool Backend::isTypeInteger(std::shared_ptr<symTable::Type> type) {
  return type && type->getName().find("integer") != std::string::npos;
}

} // namespace gazprea::backend
