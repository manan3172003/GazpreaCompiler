#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitConditional(std::shared_ptr<ast::statements::ConditionalAst> ctx) {
  visit(ctx->getCondition());
  auto [_, condAddr] = popElementFromStack(ctx->getCondition());
  auto condLoad = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), condAddr);
  auto condValue = condLoad.getResult();

  auto *currentBlock = builder->getInsertionBlock();
  auto *parentRegion = currentBlock->getParent();

  auto appendBlock = [&](mlir::Block *block) {
    parentRegion->push_back(block);
    return block;
  };

  const bool hasElse = static_cast<bool>(ctx->getElseBody());

  auto *thenBlock = new mlir::Block();
  auto *afterBlock = new mlir::Block();
  mlir::Block *elseBlock = hasElse ? new mlir::Block() : afterBlock;

  builder->setInsertionPointToEnd(currentBlock);
  builder->create<mlir::cf::CondBranchOp>(loc, condValue, thenBlock, elseBlock);

  appendBlock(thenBlock);
  if (hasElse) {
    appendBlock(elseBlock);
  }
  appendBlock(afterBlock);

  auto blockNeedsTerminator = [](mlir::Block *block) {
    return block && (block->empty() || !block->back().hasTrait<mlir::OpTrait::IsTerminator>());
  };

  builder->setInsertionPointToStart(thenBlock);
  visit(ctx->getThenBody());
  mlir::Block *thenContinuation = builder->getInsertionBlock();
  const bool thenFallsThrough = thenContinuation && !thenContinuation->hasNoPredecessors() &&
                                blockNeedsTerminator(thenContinuation);
  if (thenFallsThrough) {
    builder->setInsertionPointToEnd(thenContinuation);
    builder->create<mlir::cf::BranchOp>(loc, afterBlock);
  }

  bool elseFallsThrough = !hasElse;
  if (hasElse) {
    mlir::Block *elseContinuation = nullptr;
    builder->setInsertionPointToStart(elseBlock);
    visit(ctx->getElseBody());
    elseContinuation = builder->getInsertionBlock();
    elseFallsThrough = elseContinuation && !elseContinuation->hasNoPredecessors() &&
                       blockNeedsTerminator(elseContinuation);
    if (elseFallsThrough) {
      builder->setInsertionPointToEnd(elseContinuation);
      builder->create<mlir::cf::BranchOp>(loc, afterBlock);
    }
  }

  const bool mergeNeeded = thenFallsThrough || elseFallsThrough || !afterBlock->hasNoPredecessors();
  if (mergeNeeded) {
    builder->setInsertionPointToStart(afterBlock);
  } else {
    // Neither branch reaches the merge block and it stays unreachable.
    afterBlock->erase();
  }

  return {};
}

} // namespace gazprea::backend
