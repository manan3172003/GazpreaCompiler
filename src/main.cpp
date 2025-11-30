#include "../include/backend/Backend.h"
#include "ANTLRFileStream.h"
#include "CommonTokenStream.h"
#include "ErrorListener.h"
#include "GazpreaLexer.h"
#include "GazpreaParser.h"
#include "ast/RootAst.h"
#include "ast/walkers/AstBuilder.h"
#include "ast/walkers/DefRefWalker.h"
#include "ast/walkers/ValidationWalker.h"
#include "tree/ParseTree.h"
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Missing required argument.\n"
              << "Required arguments: <input file path> <output file path>\n";
    return 1;
  }

  // Open the file then parse and lex it.
  antlr4::ANTLRFileStream afs;
  afs.loadFromFile(argv[1]);
  gazprea::GazpreaLexer lexer(&afs);
  antlr4::CommonTokenStream tokens(&lexer);

  try {
    gazprea::GazpreaParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(new gazprea::ErrorListener());

    antlr4::tree::ParseTree *tree = parser.file();

    gazprea::ast::walkers::AstBuilder astBuilder;
    auto rootAst = std::any_cast<std::shared_ptr<gazprea::ast::RootAst>>(astBuilder.visit(tree));

    // std::cout << rootAst->toStringTree("") << std::endl;

    auto symTab = std::make_shared<gazprea::symTable::SymbolTable>();
    gazprea::ast::walkers::DefRefWalker defineWalker(symTab);
    defineWalker.visit(rootAst);

    // std::cout << rootAst->toStringTree("") << std::endl;

    gazprea::ast::walkers::ValidationWalker validationWalker(symTab);
    validationWalker.visit(rootAst);

    // std::cout << rootAst->toStringTree("") << std::endl;

    std::ofstream os(argv[2]);
    gazprea::backend::Backend backend(rootAst);
    backend.emitModule();
    backend.lowerDialects();
    backend.dumpLLVM(os);
  } catch (const std::exception &e) {
    std::cerr << e.what();
    return 1;
  }

  return 0;
}
