#include "backend/Backend.h"

namespace gazprea::backend {

std::any
Backend::visitChar(std::shared_ptr<ast::expressions::CharLiteralAst> ctx) {
  return {};
}

} // namespace gazprea::backend
