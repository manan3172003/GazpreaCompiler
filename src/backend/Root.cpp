#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitRoot(std::shared_ptr<ast::RootAst> ctx) {
  for (const auto &child : ctx->children) {
    visit(child);
  }
  return {};
}

} // namespace gazprea::backend
