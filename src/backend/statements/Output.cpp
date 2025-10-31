#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitOutput(std::shared_ptr<ast::statements::OutputAst> ctx) {
  return {};
}

} // namespace gazprea::backend
