#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitReturn(std::shared_ptr<ast::statements::ReturnAst> ctx) {
  return {};
}

} // namespace gazprea::backend
