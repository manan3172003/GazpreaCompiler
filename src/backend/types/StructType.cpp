#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitStructType(std::shared_ptr<ast::types::StructTypeAst> ctx) {
  for (auto &type : ctx->getTypes()) {
    visit(type);
  }
  return {};
}

} // namespace gazprea::backend