#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitContinue(std::shared_ptr<ast::statements::ContinueAst> ctx) {
  if (loopStack.empty()) {
    return {};
  }

  auto *continueBlock = loopStack.back().continueBlock;
  builder->create<mlir::cf::BranchOp>(loc, continueBlock);

  return {};
}

} // namespace gazprea::backend
