#include <backend/Backend.h>

namespace gazprea::backend {

std::any Backend::visitAssignment(std::shared_ptr<ast::statements::AssignmentAst> ctx) {
  return {};
}

} // namespace gazprea::backend