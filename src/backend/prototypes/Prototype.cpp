#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitPrototype(std::shared_ptr<ast::prototypes::PrototypeAst> ctx) {
  if (ctx->getReturnType())
    visit(ctx->getReturnType());
  for (const auto &param : ctx->getParams()) {
    visit(param);
  }
  return {};
}

} // namespace gazprea::backend
