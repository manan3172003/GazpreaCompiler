#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/VectorTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitIteratorLoop(std::shared_ptr<ast::statements::IteratorLoopAst> ctx) {
  auto domainExpr = ctx->getDomain();

  visit(domainExpr);
  auto [domainType, domainArrayAddr] = domainExpr->getScope()->getTopElementInStack();
  domainExpr->getScope()->popElementFromScopeStack();

  auto domainArrayType = getMLIRType(domainType);

  // Handle both arrays and vectors
  mlir::Value domainSizeAddr, domainDataPtrAddr, domainSize, domainDataPtr;
  auto vectorTypeSymbol = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(domainType);

  if (vectorTypeSymbol) {
    domainSizeAddr = gepOpVector(domainArrayType, domainArrayAddr, VectorOffset::Size);
    domainSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), domainSizeAddr);
    domainDataPtrAddr = gepOpVector(domainArrayType, domainArrayAddr, VectorOffset::Data);
    domainDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), domainDataPtrAddr);
  } else {
    domainSizeAddr = getArraySizeAddr(*builder, loc, domainArrayType, domainArrayAddr);
    domainSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), domainSizeAddr);
    domainDataPtrAddr = getArrayDataAddr(*builder, loc, domainArrayType, domainArrayAddr);
    domainDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), domainDataPtrAddr);
  }

  std::shared_ptr<symTable::Type> elementType;
  auto arrayTypeSymbol = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(domainType);

  if (arrayTypeSymbol) {
    elementType = arrayTypeSymbol->getType();
  } else if (vectorTypeSymbol) {
    elementType = vectorTypeSymbol->getType();
  }

  auto elementMLIRType = getMLIRType(elementType);

  std::string iteratorName = domainExpr->getIteratorName();

  auto *currentBlock = builder->getInsertionBlock();
  auto *parentRegion = currentBlock->getParent();

  auto appendBlock = [&](mlir::Block *block) {
    parentRegion->push_back(block);
    return block;
  };

  auto *conditionBlock = new mlir::Block();
  auto *bodyBlock = new mlir::Block();
  auto *incrementBlock = new mlir::Block();
  auto *exitBlock = new mlir::Block();

  appendBlock(conditionBlock);
  appendBlock(bodyBlock);
  appendBlock(incrementBlock);
  appendBlock(exitBlock);

  auto loopIdxAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne(), 0);
  builder->create<mlir::LLVM::StoreOp>(loc, constZero(), loopIdxAddr);

  loopStack.push_back({exitBlock, incrementBlock});

  builder->create<mlir::cf::BranchOp>(loc, conditionBlock);

  builder->setInsertionPointToStart(conditionBlock);
  auto loopIdx = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), loopIdxAddr);
  auto cond =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, loopIdx, domainSize);
  builder->create<mlir::cf::CondBranchOp>(loc, cond, bodyBlock, exitBlock);

  builder->setInsertionPointToStart(bodyBlock);
  auto currentLoopIdx = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), loopIdxAddr);
  auto domainElementPtr = builder->create<mlir::LLVM::GEPOp>(
      loc, ptrTy(), elementMLIRType, domainDataPtr, mlir::ValueRange{currentLoopIdx});
  auto domainElementValue =
      builder->create<mlir::LLVM::LoadOp>(loc, elementMLIRType, domainElementPtr);
  auto iteratorAddr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), elementMLIRType, constOne(), 0);
  builder->create<mlir::LLVM::StoreOp>(loc, domainElementValue, iteratorAddr);
  blockArg[iteratorName] = iteratorAddr;

  visit(ctx->getBody());

  blockArg.erase(iteratorName);

  mlir::Block *bodyContinuation = builder->getInsertionBlock();
  if (bodyContinuation && bodyContinuation != exitBlock &&
      (bodyContinuation->empty() ||
       !bodyContinuation->back().hasTrait<mlir::OpTrait::IsTerminator>())) {
    builder->setInsertionPointToEnd(bodyContinuation);
    builder->create<mlir::cf::BranchOp>(loc, incrementBlock);
  }

  builder->setInsertionPointToStart(incrementBlock);
  auto currentIdx = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), loopIdxAddr);
  auto nextIdx = builder->create<mlir::LLVM::AddOp>(loc, currentIdx, constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, nextIdx, loopIdxAddr);
  builder->create<mlir::cf::BranchOp>(loc, conditionBlock);

  loopStack.pop_back();

  if (exitBlock->hasNoPredecessors()) {
    builder->setInsertionPointToEnd(exitBlock);
    if (exitBlock->empty() || !exitBlock->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
      builder->create<mlir::LLVM::UnreachableOp>(loc);
    }
  }

  builder->setInsertionPointToStart(exitBlock);
  freeAllocatedMemory(domainType, domainArrayAddr);

  return {};
}

} // namespace gazprea::backend
