#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitInteger(
    std::shared_ptr<ast::expressions::IntegerLiteralAst> ctx) {
  return {};
}

} // namespace gazprea::backend
