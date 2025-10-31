#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitIdentifierLeft(std::shared_ptr<ast::statements::IdentifierLeftAst> ctx) {
  return {};
}

} // namespace gazprea::backend
