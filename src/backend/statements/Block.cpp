#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBlock(std::shared_ptr<ast::statements::BlockAst> ctx) {
  for (const auto &child : ctx->getChildren()) {
    visit(child);

    // Stop visiting further statements if a terminator was created
    auto *insertionBlock = builder->getInsertionBlock();
    if (insertionBlock && !insertionBlock->empty() &&
        insertionBlock->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
      break;
    }
  }
  auto *cleanupBlock = builder->getInsertionBlock();
  const bool hasTerminator = cleanupBlock && !cleanupBlock->empty() &&
                             cleanupBlock->back().hasTrait<mlir::OpTrait::IsTerminator>();

  if (hasTerminator) {
    auto savedInsertionPoint = builder->saveInsertionPoint();
    builder->setInsertionPoint(cleanupBlock->getTerminator());
    if (ctx->getChildren().size())
      freeElementsFromMemory(ctx->getChildren()[0]);
    if (ctx->getScope()->getScopeType() == symTable::ScopeType::Procedure ||
        ctx->getScope()->getScopeType() == symTable::ScopeType::Function) {
      freeElementsFromMemory(ctx);
    }
    builder->restoreInsertionPoint(savedInsertionPoint);
  } else {
    if (ctx->getChildren().size())
      freeElementsFromMemory(ctx->getChildren()[0]);
    if (ctx->getScope()->getScopeType() == symTable::ScopeType::Procedure ||
        ctx->getScope()->getScopeType() == symTable::ScopeType::Function) {
      freeElementsFromMemory(ctx);
    }
  }
  return {};
}
} // namespace gazprea::backend
