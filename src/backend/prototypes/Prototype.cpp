#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitPrototype(std::shared_ptr<ast::prototypes::PrototypeAst> ctx) {
  for (const auto &param : ctx->getParams()) {
    visit(param);
  }
  return {};
}

} // namespace gazprea::backend
