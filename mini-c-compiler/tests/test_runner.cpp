#include "../lexer.h"
#include "../parser.h"
#include "../semantic.h"
#include "../ir.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct TestCase {
    std::string name;
    std::string filepath;
    bool shouldSucceed;
    std::string expectedErrorSubstring;
};

std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open test file: " + filepath);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool runTestCase(const TestCase& tc, std::string& failureReason) {
    std::string source;
    try {
        source = readFile(tc.filepath);
    } catch (const std::exception& e) {
        failureReason = e.what();
        return false;
    }

    // 1. Lexer
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    if (lexer.hasErrors()) {
        if (!tc.shouldSucceed) {
            for (const auto& err : lexer.getErrors()) {
                if (err.find(tc.expectedErrorSubstring) != std::string::npos) {
                    return true; // Expected error caught correctly
                }
            }
            failureReason = "Lexer produced errors, but did not match expected: " + tc.expectedErrorSubstring;
            return false;
        } else {
            failureReason = "Unexpected Lexer error: " + lexer.getErrors().front();
            return false;
        }
    }

    // 2. Parser
    Parser parser(tokens);
    auto ast = parser.parse();
    if (parser.hasErrors()) {
        if (!tc.shouldSucceed) {
            for (const auto& err : parser.getErrors()) {
                if (err.find(tc.expectedErrorSubstring) != std::string::npos) {
                    return true; // Expected syntax error caught correctly
                }
            }
            failureReason = "Parser produced errors, but did not match expected: " + tc.expectedErrorSubstring;
            return false;
        } else {
            failureReason = "Unexpected Parser error: " + parser.getErrors().front();
            return false;
        }
    }

    // 3. Semantic Analysis
    SemanticAnalyzer semantic;
    bool semanticOk = semantic.analyze(*ast);
    if (!semanticOk) {
        if (!tc.shouldSucceed) {
            for (const auto& err : semantic.getErrors()) {
                if (err.find(tc.expectedErrorSubstring) != std::string::npos) {
                    return true; // Expected semantic error caught correctly
                }
            }
            failureReason = "Semantic analyzer produced errors, but did not match expected: " + tc.expectedErrorSubstring;
            return false;
        } else {
            failureReason = "Unexpected Semantic error: " + semantic.getErrors().front();
            return false;
        }
    }

    // If it was supposed to fail, but reached here without failing
    if (!tc.shouldSucceed) {
        failureReason = "Test expected failure (" + tc.expectedErrorSubstring + "), but compilation succeeded!";
        return false;
    }

    // 4. Optimization & IR Generation
    optimizeProgram(*ast);
    IRGenerator irGen;
    irGen.generate(*ast);
    std::string irCode = irGen.getIRString();

    // Specific functional assertions
    if (tc.name.find("Constant Folding") != std::string::npos) {
        if (irCode.find("folded = 14") == std::string::npos) {
            failureReason = "Constant folding failed to fold 2 + 3 * 4 into 14. IR:\n" + irCode;
            return false;
        }
    }

    if (tc.name.find("Division by Zero") != std::string::npos) {
        // Must contain safe unfolded division instruction "10 / 0"
        if (irCode.find("/ 0") == std::string::npos) {
            failureReason = "Division by zero was unexpectedly altered. IR:\n" + irCode;
            return false;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    std::string basePath = "tests/";
    if (argc > 1) {
        basePath = argv[1];
        if (!basePath.empty() && basePath.back() != '/' && basePath.back() != '\\') {
            basePath += "/";
        }
    }

    std::vector<TestCase> testCases = {
        {"01_Valid_Declarations",    basePath + "01_valid_basic.mc",          true,  ""},
        {"02_Operator_Precedence",   basePath + "02_operator_precedence.mc", true,  ""},
        {"03_Constant_Folding",      basePath + "03_constant_folding.mc",    true,  ""},
        {"04_Undeclared_Variable",   basePath + "04_undeclared_variable.mc", false, "undeclared"},
        {"05_Duplicate_Declaration", basePath + "05_duplicate_declaration.mc", false, "Redefinition"},
        {"06_Missing_Semicolon",     basePath + "06_missing_semicolon.mc",   false, "Expected ';'"},
        {"07_Invalid_Character",     basePath + "07_invalid_char.mc",        false, "Invalid character"},
        {"08_Division_by_Zero_Safe", basePath + "08_div_by_zero.mc",         true,  ""}
    };

    std::cout << "=========================================================\n";
    std::cout << "          Mini C Compiler Automated Test Suite          \n";
    std::cout << "=========================================================\n";

    int passed = 0;
    int failed = 0;

    for (const auto& tc : testCases) {
        std::string failureReason;
        bool ok = runTestCase(tc, failureReason);

        std::cout << "[";
        if (ok) {
            std::cout << "PASS";
            passed++;
        } else {
            std::cout << "FAIL";
            failed++;
        }
        std::cout << "] " << tc.name;

        if (!ok) {
            std::cout << " --> Reason: " << failureReason;
        }
        std::cout << "\n";
    }

    std::cout << "=========================================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed, "
              << testCases.size() << " total.\n";
    std::cout << "=========================================================\n";

    return (failed == 0) ? 0 : 1;
}
