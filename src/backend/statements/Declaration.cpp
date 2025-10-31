#include "backend/Backend.h"

namespace gazprea::backend {
std::any Backend::visitDeclaration(std::shared_ptr<ast::statements::DeclarationAst> ctx) {
  return {};
}

} // namespace gazprea::backend