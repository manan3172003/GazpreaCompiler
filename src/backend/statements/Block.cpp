#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBlock(std::shared_ptr<ast::statements::BlockAst> ctx) {
  for (const auto &child : ctx->getChildren()) {
    visit(child);
  }
  return {};
}
} // namespace gazprea::backend
