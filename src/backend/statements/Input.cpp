#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitInput(std::shared_ptr<ast::statements::InputAst> ctx) {
  visit(ctx->getLVal());
  // auto [type, valueAddr] = popElementFromStack(ctx->getLVal());

  return {};
}
} // namespace gazprea::backend
