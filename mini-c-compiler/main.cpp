#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "ir.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

void printUsage(const char* progName) {
    std::cout << "Mini C Compiler (C++17)\n";
    std::cout << "Usage: " << progName << " <source_file.mc> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --dump-tokens    Display the token stream from the Lexer\n";
    std::cout << "  --dump-ast       Display the Abstract Syntax Tree (AST)\n";
    std::cout << "  --no-opt         Disable compile-time constant folding\n";
    std::cout << "  --help, -h       Display this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string filename;
    bool dumpTokens = false;
    bool dumpAST = false;
    bool optimize = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-tokens") {
            dumpTokens = true;
        } else if (arg == "--dump-ast") {
            dumpAST = true;
        } else if (arg == "--no-opt") {
            optimize = false;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown flag: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        } else {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::cerr << "Error: No input file specified.\n";
        printUsage(argv[0]);
        return 1;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    // ========================================================================
    // 1. Lexical Analysis
    // ========================================================================
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    if (dumpTokens) {
        std::cout << "=== TOKEN STREAM ===\n";
        for (const auto& tok : tokens) {
            std::cout << "Line " << tok.line << ":" << tok.column << " ["
                      << tokenTypeToString(tok.type) << "] "
                      << (tok.lexeme.empty() ? "" : "'" + tok.lexeme + "'") << "\n";
        }
        std::cout << "====================\n\n";
    }

    if (lexer.hasErrors()) {
        for (const auto& err : lexer.getErrors()) {
            std::cerr << err << "\n";
        }
        return 1;
    }

    // ========================================================================
    // 2. Syntax Analysis (Parser)
    // ========================================================================
    Parser parser(tokens);
    auto ast = parser.parse();

    if (parser.hasErrors()) {
        for (const auto& err : parser.getErrors()) {
            std::cerr << err << "\n";
        }
        return 1;
    }

    // ========================================================================
    // 3. Semantic Analysis (Symbol Table & Scope Verification)
    // ========================================================================
    SemanticAnalyzer semantic;
    if (!semantic.analyze(*ast)) {
        for (const auto& err : semantic.getErrors()) {
            std::cerr << err << "\n";
        }
        return 1;
    }

    // ========================================================================
    // 4. Optimization (AST Constant Folding)
    // ========================================================================
    if (optimize) {
        optimizeProgram(*ast);
    }

    if (dumpAST) {
        std::cout << "=== ABSTRACT SYNTAX TREE ===\n";
        ast->print(std::cout);
        std::cout << "============================\n\n";
    }

    // ========================================================================
    // 5. Intermediate Representation (Three-Address Code) Generation
    // ========================================================================
    IRGenerator irGen;
    irGen.generate(*ast);
    irGen.print(std::cout);

    return 0;
}
