#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitLoop(std::shared_ptr<ast::statements::LoopAst> ctx) {
  const bool isDoWhile = ctx->getIsPostPredicated();
  const bool hasExplicitCondition = !ctx->getIsInfinite() && ctx->getCondition() != nullptr;

  auto *currentBlock = builder->getInsertionBlock();
  auto *parentRegion = currentBlock->getParent();

  auto appendBlock = [&](mlir::Block *block) {
    parentRegion->push_back(block);
    return block;
  };

  mlir::Block *conditionBlock = hasExplicitCondition ? new mlir::Block() : nullptr;
  auto *bodyBlock = new mlir::Block();
  auto *exitBlock = new mlir::Block();

  if (!isDoWhile && conditionBlock) {
    appendBlock(conditionBlock);
    appendBlock(bodyBlock);
  } else {
    appendBlock(bodyBlock);
    if (conditionBlock) {
      appendBlock(conditionBlock);
    }
  }
  appendBlock(exitBlock);

  mlir::Block *continueTarget = conditionBlock ? conditionBlock : bodyBlock;
  loopStack.push_back({exitBlock, continueTarget});

  builder->setInsertionPointToEnd(currentBlock);
  builder->create<mlir::cf::BranchOp>(loc, isDoWhile ? bodyBlock : continueTarget);

  if (!isDoWhile && conditionBlock) {
    builder->setInsertionPointToStart(conditionBlock);
    visit(ctx->getCondition());
    auto [_, condAddr] = ctx->getCondition()->getScope()->getTopElementInStack();
    auto condLoad = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), condAddr);
    auto condValue = condLoad.getResult();
    builder->create<mlir::cf::CondBranchOp>(loc, condValue, bodyBlock, exitBlock);
  }

  auto blockNeedsTerminator = [](mlir::Block *block) {
    return block && (block->empty() || !block->back().hasTrait<mlir::OpTrait::IsTerminator>());
  };

  builder->setInsertionPointToStart(bodyBlock);
  visit(ctx->getBody());

  mlir::Block *bodyContinuation = builder->getInsertionBlock();
  if (bodyContinuation && bodyContinuation != exitBlock && blockNeedsTerminator(bodyContinuation)) {
    builder->setInsertionPointToEnd(bodyContinuation);
    builder->create<mlir::cf::BranchOp>(loc, continueTarget);
  }

  if (isDoWhile && conditionBlock) {
    builder->setInsertionPointToStart(conditionBlock);
    visit(ctx->getCondition());
    auto [_, condAddr] = ctx->getCondition()->getScope()->getTopElementInStack();
    auto condLoad = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), condAddr);
    auto condValue = condLoad.getResult();
    builder->create<mlir::cf::CondBranchOp>(loc, condValue, bodyBlock, exitBlock);
  }

  loopStack.pop_back();

  if (exitBlock->hasNoPredecessors()) {
    builder->setInsertionPointToEnd(exitBlock);
    builder->create<mlir::LLVM::UnreachableOp>(loc);
  }
  builder->setInsertionPointToStart(exitBlock);

  return {};
}

} // namespace gazprea::backend
