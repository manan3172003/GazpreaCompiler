#include "symTable/VariableSymbol.h"
#include <backend/Backend.h>

namespace gazprea::backend {
std::any Backend::visitLenMemberFunc(std::shared_ptr<ast::statements::LenMemberFuncAst> ctx) {
  visit(ctx->getLeft());

  auto [vectorType, vectorAddr] = popElementFromStack(ctx);
  if (auto varSym =
          std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getLeft()->getSymbol())) {
    vectorType = varSym->getType();
    vectorAddr = varSym->value;
  }
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(vectorType);
  if (!vectorTypeSym) {
    return {};
  }

  auto lenFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("@vector_len");
  if (!lenFunc) {
    makeLenMemberFunc();
    lenFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("@vector_len");
  }

  auto lenCall = builder->create<mlir::LLVM::CallOp>(loc, lenFunc, mlir::ValueRange{vectorAddr});
  auto lenValue = lenCall.getResult();

  auto lenAlloca = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, lenValue, lenAlloca);

  auto integerType =
      std::dynamic_pointer_cast<symTable::Type>(ctx->getScope()->resolveType("integer"));
  if (integerType) {
    ctx->getScope()->pushElementToScopeStack(integerType, lenAlloca);
  }

  return {};
}
std::any Backend::visitAppendMemberFunc(std::shared_ptr<ast::statements::AppendMemberFuncAst> ctx) {
  return {};
}
std::any Backend::visitPushMemberFunc(std::shared_ptr<ast::statements::PushMemberFuncAst> ctx) {
  visit(ctx->getLeft());

  auto [vectorType, vectorAddr] = popElementFromStack(ctx);
  if (auto varSym =
          std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getLeft()->getSymbol())) {
    vectorType = varSym->getType();
    vectorAddr = varSym->value;
  }
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(vectorType);
  if (!vectorTypeSym) {
    return {};
  }

  const auto elementType = vectorTypeSym->getType();
  auto elementMLIRType = getMLIRType(elementType);
  if (!elementMLIRType) {
    return {};
  }

  auto pushFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("@vector_push");
  if (!pushFunc) {
    makePushMemberFunc();
    pushFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("@vector_push");
  }

  auto freeFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("free");
  if (!freeFunc) {
    auto savedInsertionPoint = builder->saveInsertionPoint();
    builder->setInsertionPointToStart(module.getBody());

    auto voidType = mlir::LLVM::LLVMVoidType::get(builder->getContext());
    auto freeFnType = mlir::LLVM::LLVMFunctionType::get(voidType, {ptrTy()}, false);
    freeFunc = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "free", freeFnType);

    builder->restoreInsertionPoint(savedInsertionPoint);
  }

  auto vectorStructType = getMLIRType(vectorTypeSym);
  auto sizeAddr = gepOpVector(vectorStructType, vectorAddr, VectorOffset::Size);
  auto capacityAddr = gepOpVector(vectorStructType, vectorAddr, VectorOffset::Capacity);
  auto dataAddr = gepOpVector(vectorStructType, vectorAddr, VectorOffset::Data);

  auto zeroConst = constZero();
  auto oneConst = constOne();

  for (const auto &arg : ctx->getArgs()) {
    visit(arg);
    auto [argType, argAddr] = popElementFromStack(arg);
    if (!argType) {
      continue;
    }

    auto currentSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizeAddr);
    auto newSize = builder->create<mlir::LLVM::AddOp>(loc, intTy(), currentSize, oneConst);

    auto oldDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr);
    auto newDataPtr = mallocArray(elementMLIRType, newSize);

    builder->create<mlir::scf::ForOp>(
        loc, zeroConst, currentSize, oneConst, mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto srcPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, oldDataPtr,
                                                    mlir::ValueRange{i});
          auto destPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, newDataPtr,
                                                     mlir::ValueRange{i});
          auto elemValue = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, srcPtr);
          b.create<mlir::LLVM::StoreOp>(l, elemValue, destPtr);
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    auto insertPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), elementMLIRType, newDataPtr,
                                                        mlir::ValueRange{currentSize});
    copyValue(elementType, argAddr, insertPtr);

    auto currentCapacity = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), capacityAddr);
    auto hasAlloc = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sgt,
                                                        currentCapacity, zeroConst);

    builder->create<mlir::scf::IfOp>(
        loc, hasAlloc,
        [&](mlir::OpBuilder &b, mlir::Location l) {
          b.create<mlir::LLVM::CallOp>(l, freeFunc, mlir::ValueRange{oldDataPtr});
          b.create<mlir::scf::YieldOp>(l);
        },
        [&](mlir::OpBuilder &b, mlir::Location l) { b.create<mlir::scf::YieldOp>(l); });

    builder->create<mlir::LLVM::CallOp>(loc, pushFunc,
                                        mlir::ValueRange{vectorAddr, newDataPtr, newSize});
  }

  return {};
}
std::any Backend::visitConcatMemberFunc(std::shared_ptr<ast::statements::ConcatMemberFuncAst> ctx) {
  return {};
}
} // namespace gazprea::backend
