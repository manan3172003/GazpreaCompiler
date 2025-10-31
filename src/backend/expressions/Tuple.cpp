#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitTuple(std::shared_ptr<ast::expressions::TupleLiteralAst> ctx) { return {}; }

} // namespace gazprea::backend
