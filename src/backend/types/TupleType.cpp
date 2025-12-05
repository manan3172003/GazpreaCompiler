#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitTupleType(std::shared_ptr<ast::types::TupleTypeAst> ctx) {
  for (auto &type : ctx->getTypes()) {
    visit(type);
  }
  return {};
}

} // namespace gazprea::backend
