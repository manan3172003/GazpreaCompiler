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
  ctx->getScope()->getScopeStack().clear();
  return {};
}
} // namespace gazprea::backend
