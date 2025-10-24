#include "GazpreaLexer.h"
#include "GazpreaParser.h"

#include "ANTLRFileStream.h"
#include "CommonTokenStream.h"
#include "tree/ParseTree.h"
#include "tree/ParseTreeWalker.h"

#include "BackEnd.h"
#include "ast/RootAst.h"
#include "ast/statements/DeclarationAst.h"
#include "ast/walkers/AstBuilder.h"

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
  gazprea::GazpreaParser parser(&tokens);

  antlr4::tree::ParseTree *tree = parser.file();

  gazprea::ast::walkers::AstBuilder astBuilder;
  auto rootAst = std::any_cast<std::shared_ptr<gazprea::ast::RootAst>>(
      astBuilder.visit(tree));

  std::cout << rootAst->toStringTree("") << std::endl;

  std::ofstream os(argv[2]);
  // BackEnd backend;
  // backend.emitModule();
  // backend.lowerDialects();
  // backend.dumpLLVM(os);

  return 0;
}
