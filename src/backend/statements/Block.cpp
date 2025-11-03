#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBlock(std::shared_ptr<ast::statements::BlockAst> ctx) {
  for (const auto &child : ctx->getChildren()) {
    visit(child);

    // Stop visiting further statements if a terminator was created
    if (!builder->getBlock()->empty() &&
        (mlir::isa<mlir::scf::YieldOp>(builder->getBlock()->back()) ||
         mlir::isa<mlir::LLVM::ReturnOp>(builder->getBlock()->back()))) {
      break;
    }
  }
  ctx->getScope()->getScopeStack().clear();
  return {};
}
} // namespace gazprea::backend
