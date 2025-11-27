#include "backend/Backend.h"
#include "symTable/MethodSymbol.h"

namespace gazprea::backend {

std::any
Backend::visitStructFuncCallRouter(std::shared_ptr<ast::expressions::StructFuncCallRouterAst> ctx) {
  // if (ctx->getIsStruct()) visit(ctx->getStructLiteralAst());
  // else
  visit(ctx->getFuncProcCallAst());
  return {};
}

} // namespace gazprea::backend