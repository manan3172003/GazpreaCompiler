#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitTupleAccess(
    std::shared_ptr<ast::expressions::TupleAccessAst> ctx) {
  return {};
}

} // namespace gazprea::backend
