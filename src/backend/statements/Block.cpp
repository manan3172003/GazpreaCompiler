#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBlock(std::shared_ptr<ast::statements::BlockAst> ctx) {
  for (const auto &child : ctx->getChildren()) {
    visit(child);
  }
  ctx->getScope()->getScopeStack().clear();
  return {};
}
} // namespace gazprea::backend
