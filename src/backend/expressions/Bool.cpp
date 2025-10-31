#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBool(std::shared_ptr<ast::expressions::BoolLiteralAst> ctx) { return {}; }

} // namespace gazprea::backend
