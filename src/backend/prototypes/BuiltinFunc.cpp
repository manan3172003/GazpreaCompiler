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
std::any
Backend::visitShapeBuiltinFunc(std::shared_ptr<ast::expressions::ShapeBuiltinFuncAst> ctx) {
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
