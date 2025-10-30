#include "CompileTimeExceptions.h"
#include "GazpreaLexer.h"

#include <ErrorListener.h>
#include <Parser.h>
#include <Token.h>

namespace gazprea {
void ErrorListener::syntaxError(antlr4::Recognizer *recognizer,
                                antlr4::Token *offendingSymbol, size_t line,
                                size_t charPositionInLine,
                                const std::string &msg, std::exception_ptr e) {
  const std::vector<std::string> rule_stack =
      static_cast<antlr4::Parser *>(recognizer)->getRuleInvocationStack();

  switch (offendingSymbol->getType()) {
  case GazpreaLexer::FUNCTION: {
    // Check if function was declared somewhere except global scope
    for (const auto &rule : rule_stack) {
      if (rule == "procedure_stat" || rule == "block_stat" || rule == "function_stat") {
        throw StatementError(
            line, "Function declarations are only allowed in global scope");
      }
    }
  }
  case GazpreaLexer::PROCEDURE: {
    // Check if procedure was declared somewhere except global scope
    for (const auto &rule : rule_stack) {
      if (rule == "procedure_stat" || rule == "block_stat" || rule == "function_stat") {
        throw StatementError(
            line, "Procedure declarations are only allowed in global scope");
      }
    }
  }
  // TODO: Implement other syntax checks here for other statements
  default:;
  }

  throw SyntaxError(line, msg);
}
} // namespace gazprea