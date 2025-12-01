#include "symTable/VariableSymbol.h"
#include <backend/Backend.h>

namespace gazprea::backend {
void Backend::makeLengthBuiltin() {
  if (module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("length")) {
    return;
  }

  auto savedInsertPoint = builder->saveInsertionPoint();
  builder->setInsertionPointToStart(module.getBody());

  auto lenType = mlir::LLVM::LLVMFunctionType::get(intTy(), {ptrTy(), intTy()}, false);
  auto lenFunc = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "length", lenType);
  auto *entry = lenFunc.addEntryBlock();
  builder->setInsertionPointToStart(entry);

  auto valuePtr = entry->getArgument(0);
  auto typeCode = entry->getArgument(1);

  auto arrayStructTy = arrayTy();
  auto vectorStructTy = vectorTy();

  auto idxTy = builder->getI32Type();
  auto zeroIdx = builder->create<mlir::LLVM::ConstantOp>(loc, idxTy, 0);
  auto vectorCode = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 1);

  auto isVector =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, typeCode, vectorCode);

  auto sizeValue = builder->create<mlir::scf::IfOp>(
      loc, isVector,
      [&](mlir::OpBuilder &b, mlir::Location l) {
        auto vecSizePtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), vectorStructTy, valuePtr,
                                                      mlir::ValueRange{zeroIdx, zeroIdx});
        auto vecSize = b.create<mlir::LLVM::LoadOp>(l, intTy(), vecSizePtr);
        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{vecSize});
      },
      [&](mlir::OpBuilder &b, mlir::Location l) {
        auto arraySizePtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), arrayStructTy, valuePtr,
                                                        mlir::ValueRange{zeroIdx, zeroIdx});
        auto arraySize = b.create<mlir::LLVM::LoadOp>(l, intTy(), arraySizePtr);
        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{arraySize});
      });

  builder->create<mlir::LLVM::ReturnOp>(loc, sizeValue.getResult(0));

  builder->restoreInsertionPoint(savedInsertPoint);
}
std::any
Backend::visitLengthBuiltinFunc(std::shared_ptr<ast::expressions::LengthBuiltinFuncAst> ctx) {
  visit(ctx->arg);

  auto [argType, argAddr] = popElementFromStack(ctx);
  if (auto varSym = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->arg->getSymbol())) {
    argType = varSym->getType();
    argAddr = varSym->value;
  }

  if (!argType || !argAddr) {
    return {};
  }

  const auto typeName = argType->getName();
  const bool isVector = typeName.rfind("vector", 0) == 0;
  const bool isArray = typeName.rfind("array", 0) == 0;
  if (!isVector && !isArray) {
    return {};
  }

  auto lenFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("length");
  if (!lenFunc) {
    makeLengthBuiltin();
    lenFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("length");
  }

  auto typeCode = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), isVector ? 1 : 0);
  auto callOp =
      builder->create<mlir::LLVM::CallOp>(loc, lenFunc, mlir::ValueRange{argAddr, typeCode});

  auto resultAlloca = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, callOp.getResult(), resultAlloca);
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), resultAlloca);

  return {};
}
void Backend::makeShapeBuiltin() {
  if (module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("shape")) {
    return;
  }

  auto savedInsertPoint = builder->saveInsertionPoint();
  builder->setInsertionPointToStart(module.getBody());

  auto resultTy = arrayTy();
  auto shapeType = mlir::LLVM::LLVMFunctionType::get(resultTy, {ptrTy(), intTy()}, false);
  auto shapeFunc = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "shape", shapeType);
  auto *entry = shapeFunc.addEntryBlock();
  builder->setInsertionPointToStart(entry);

  auto valuePtr = entry->getArgument(0);
  auto typeCode = entry->getArgument(1);

  auto arrayStructTy = arrayTy();
  auto vectorStructTy = vectorTy();

  auto idxTy = builder->getI32Type();
  auto zeroIdx = builder->create<mlir::LLVM::ConstantOp>(loc, idxTy, 0);
  auto oneIdx = builder->create<mlir::LLVM::ConstantOp>(loc, idxTy, 1);
  auto twoIdx = builder->create<mlir::LLVM::ConstantOp>(loc, idxTy, 2);
  auto vectorCode = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 1);
  auto zeroInt = constZero();
  auto oneInt = constOne();
  auto twoInt = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 2);
  auto boolFalse = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);

  auto emitShapeStruct = [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value count,
                             const std::vector<mlir::Value> &dims) -> mlir::Value {
    auto dataPtr = mallocArray(intTy(), count);
    for (size_t i = 0; i < dims.size(); ++i) {
      auto idxConst = b.create<mlir::LLVM::ConstantOp>(l, intTy(), static_cast<int>(i));
      auto elemPtr =
          b.create<mlir::LLVM::GEPOp>(l, ptrTy(), intTy(), dataPtr, mlir::ValueRange{idxConst});
      b.create<mlir::LLVM::StoreOp>(l, dims[i], elemPtr);
    }
    auto resultAlloca = b.create<mlir::LLVM::AllocaOp>(l, ptrTy(), arrayStructTy, constOne());
    auto sizePtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), arrayStructTy, resultAlloca,
                                               mlir::ValueRange{zeroIdx, zeroIdx});
    b.create<mlir::LLVM::StoreOp>(l, count, sizePtr);
    auto dataFieldPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), arrayStructTy, resultAlloca,
                                                    mlir::ValueRange{zeroIdx, oneIdx});
    b.create<mlir::LLVM::StoreOp>(l, dataPtr, dataFieldPtr);
    auto boolPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), arrayStructTy, resultAlloca,
                                               mlir::ValueRange{zeroIdx, twoIdx});
    b.create<mlir::LLVM::StoreOp>(l, boolFalse, boolPtr);
    return b.create<mlir::LLVM::LoadOp>(l, arrayStructTy, resultAlloca);
  };

  auto isVector =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, typeCode, vectorCode);

  auto shapeResult = builder->create<mlir::scf::IfOp>(
      loc, isVector,
      [&](mlir::OpBuilder &b, mlir::Location l) {
        // Vector branch
        auto vecSizePtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), vectorStructTy, valuePtr,
                                                      mlir::ValueRange{zeroIdx, zeroIdx});
        auto vecSize = b.create<mlir::LLVM::LoadOp>(l, intTy(), vecSizePtr);
        auto vectorShape = emitShapeStruct(b, l, oneInt, {vecSize});
        b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{vectorShape});
      },
      [&](mlir::OpBuilder &b, mlir::Location l) {
        auto arrSizePtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), arrayStructTy, valuePtr,
                                                      mlir::ValueRange{zeroIdx, zeroIdx});
        auto outerSize = b.create<mlir::LLVM::LoadOp>(l, intTy(), arrSizePtr);
        auto dataField = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), arrayStructTy, valuePtr,
                                                     mlir::ValueRange{zeroIdx, oneIdx});
        auto arrayDataPtr = b.create<mlir::LLVM::LoadOp>(l, ptrTy(), dataField);
        auto is2dPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), arrayStructTy, valuePtr,
                                                   mlir::ValueRange{zeroIdx, twoIdx});
        auto is2dValue = b.create<mlir::LLVM::LoadOp>(l, boolTy(), is2dPtr);

        auto arrayShapeResult = b.create<mlir::scf::IfOp>(
            l, is2dValue,
            [&](mlir::OpBuilder &b2, mlir::Location l2) {
              // 2D array branch
              auto rowsIsZero = b2.create<mlir::LLVM::ICmpOp>(l2, mlir::LLVM::ICmpPredicate::eq,
                                                              outerSize, zeroInt);
              auto columnsVal = b2.create<mlir::scf::IfOp>(
                  l2, rowsIsZero,
                  [&](mlir::OpBuilder &b3, mlir::Location l3) {
                    b3.create<mlir::scf::YieldOp>(l3, mlir::ValueRange{zeroInt});
                  },
                  [&](mlir::OpBuilder &b3, mlir::Location l3) {
                    auto firstRowPtr = b3.create<mlir::LLVM::GEPOp>(
                        l3, ptrTy(), arrayStructTy, arrayDataPtr, mlir::ValueRange{zeroIdx});
                    auto firstRowSizePtr =
                        b3.create<mlir::LLVM::GEPOp>(l3, ptrTy(), arrayStructTy, firstRowPtr,
                                                     mlir::ValueRange{zeroIdx, zeroIdx});
                    auto cols = b3.create<mlir::LLVM::LoadOp>(l3, intTy(), firstRowSizePtr);
                    b3.create<mlir::scf::YieldOp>(l3, mlir::ValueRange{cols});
                  });
              auto twoDimShape =
                  emitShapeStruct(b2, l2, twoInt, {outerSize, columnsVal.getResult(0)});
              b2.create<mlir::scf::YieldOp>(l2, mlir::ValueRange{twoDimShape});
            },
            [&](mlir::OpBuilder &b2, mlir::Location l2) {
              // 1D array branch
              auto oneDimShape = emitShapeStruct(b2, l2, oneInt, {outerSize});
              b2.create<mlir::scf::YieldOp>(l2, mlir::ValueRange{oneDimShape});
            });

        b.create<mlir::scf::YieldOp>(l, arrayShapeResult.getResults());
      });

  builder->create<mlir::LLVM::ReturnOp>(loc, shapeResult.getResult(0));

  builder->restoreInsertionPoint(savedInsertPoint);
}
std::any
Backend::visitShapeBuiltinFunc(std::shared_ptr<ast::expressions::ShapeBuiltinFuncAst> ctx) {
  visit(ctx->arg);

  auto [argType, argAddr] = popElementFromStack(ctx);
  if (auto varSym = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->arg->getSymbol())) {
    argType = varSym->getType();
    argAddr = varSym->value;
  }

  if (!argType || !argAddr) {
    return {};
  }

  const auto typeName = argType->getName();
  const bool isVector = typeName.rfind("vector", 0) == 0;
  const bool isArray = typeName.rfind("array", 0) == 0;
  if (!isVector && !isArray) {
    return {};
  }

  auto shapeFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("shape");
  if (!shapeFunc) {
    makeShapeBuiltin();
    shapeFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("shape");
  }

  auto typeCode = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), isVector ? 1 : 0);
  auto callOp =
      builder->create<mlir::LLVM::CallOp>(loc, shapeFunc, mlir::ValueRange{argAddr, typeCode});

  auto resultAlloca = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayTy(), constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, callOp.getResult(), resultAlloca);
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), resultAlloca);

  return {};
}
std::any
Backend::visitReverseBuiltinFunc(std::shared_ptr<ast::expressions::ReverseBuiltinFuncAst> ctx) {
  return {};
}
std::any
Backend::visitFormatBuiltinFunc(std::shared_ptr<ast::expressions::FormatBuiltinFuncAst> ctx) {
  return {};
}
} // namespace gazprea::backend
