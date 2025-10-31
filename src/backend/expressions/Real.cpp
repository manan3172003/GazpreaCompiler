#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitReal(std::shared_ptr<ast::expressions::RealLiteralAst> ctx) { return {}; }

} // namespace gazprea::backend
