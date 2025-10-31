#include "backend/Backend.h"

namespace gazprea::backend {

std::any
Backend::visitTupleType(std::shared_ptr<ast::types::TupleTypeAst> ctx) {
  return {};
}

} // namespace gazprea::backend
