#ifndef MINI_C_SEMANTIC_H
#define MINI_C_SEMANTIC_H

#include "ast.h"
#include <string>
#include <vector>
#include <unordered_map>

// ============================================================================
// Symbol Record & Symbol Table
// ============================================================================
struct Symbol {
    std::string name;
    std::string type; // "int"
    int lineDeclared = 0;
    int columnDeclared = 0;
    bool isInitialized = false;
};

class SymbolTable {
public:
    // Returns false if variable is already declared in this scope (duplicate declaration)
    bool declare(const std::string& name, const std::string& type, int line, int col, bool isInit);

    // Checks if variable name exists in symbol table
    bool isDeclared(const std::string& name) const;

    // Lookup symbol information
    const Symbol* lookup(const std::string& name) const;

    // Mark variable as initialized
    void markInitialized(const std::string& name);

    const std::unordered_map<std::string, Symbol>& getAllSymbols() const { return table; }

private:
    std::unordered_map<std::string, Symbol> table;
};

// ============================================================================
// Semantic Analyzer
// ============================================================================
class SemanticAnalyzer {
public:
    SemanticAnalyzer() = default;

    // Validates program semantics: symbol table declarations, undeclared variables, duplicates
    bool analyze(const ProgramAST& program);

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    const SymbolTable& getSymbolTable() const { return symbolTable; }

private:
    SymbolTable symbolTable;
    std::vector<std::string> errors;

    void analyzeStatement(const StmtAST* stmt);
    void analyzeVarDecl(const VarDeclStmtAST* stmt);
    void analyzeAssign(const AssignStmtAST* stmt);
    void analyzePrint(const PrintStmtAST* stmt);
    void analyzeExpression(const ExprAST* expr);
};

#endif // MINI_C_SEMANTIC_H
