#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitTupleUnpackAssign(
    std::shared_ptr<ast::statements::TupleUnpackAssignAst> ctx) {
  return {};
}

} // namespace gazprea::backend
