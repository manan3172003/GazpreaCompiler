#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitCast(std::shared_ptr<ast::expressions::CastAst> ctx) {
  return {};
}

} // namespace gazprea::backend
