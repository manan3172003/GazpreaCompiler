#include "backend/Backend.h"

namespace gazprea::backend {

std::any
Backend::visitArrayElementAssign(std::shared_ptr<ast::statements::ArrayElementAssignAst> ctx) {
  return {};
}

} // namespace gazprea::backend