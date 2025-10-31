#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitIdentifier(std::shared_ptr<ast::expressions::IdentifierAst> ctx) {
  return {};
}

} // namespace gazprea::backend
