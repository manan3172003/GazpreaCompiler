#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitProcedureCall(
    std::shared_ptr<ast::statements::ProcedureCallAst> ctx) {
  return {};
}

} // namespace gazprea::backend
