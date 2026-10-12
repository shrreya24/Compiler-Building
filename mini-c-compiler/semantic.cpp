#include "semantic.h"

// ============================================================================
// SymbolTable Implementation
// ============================================================================
bool SymbolTable::declare(const std::string& name, const std::string& type, int line, int col, bool isInit) {
    if (table.find(name) != table.end()) {
        return false; // Duplicate declaration
    }
    Symbol sym;
    sym.name = name;
    sym.type = type;
    sym.lineDeclared = line;
    sym.columnDeclared = col;
    sym.isInitialized = isInit;
    table[name] = sym;
    return true;
}

bool SymbolTable::isDeclared(const std::string& name) const {
    return table.find(name) != table.end();
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    auto it = table.find(name);
    if (it != table.end()) {
        return &it->second;
    }
    return nullptr;
}

void SymbolTable::markInitialized(const std::string& name) {
    auto it = table.find(name);
    if (it != table.end()) {
        it->second.isInitialized = true;
    }
}

// ============================================================================
// SemanticAnalyzer Implementation
// ============================================================================
bool SemanticAnalyzer::analyze(const ProgramAST& program) {
    errors.clear();
    for (const auto& stmt : program.statements) {
        if (stmt) {
            analyzeStatement(stmt.get());
        }
    }
    return !hasErrors();
}

void SemanticAnalyzer::analyzeStatement(const StmtAST* stmt) {
    if (auto varDecl = dynamic_cast<const VarDeclStmtAST*>(stmt)) {
        analyzeVarDecl(varDecl);
    } else if (auto assign = dynamic_cast<const AssignStmtAST*>(stmt)) {
        analyzeAssign(assign);
    } else if (auto print = dynamic_cast<const PrintStmtAST*>(stmt)) {
        analyzePrint(print);
    }
}

void SemanticAnalyzer::analyzeVarDecl(const VarDeclStmtAST* stmt) {
    bool hasInit = (stmt->initExpr != nullptr);

    // If there is an initialization expression, analyze it first
    if (hasInit) {
        analyzeExpression(stmt->initExpr.get());
    }

    bool success = symbolTable.declare(stmt->name, stmt->type, stmt->line, stmt->column, hasInit);
    if (!success) {
        const Symbol* prev = symbolTable.lookup(stmt->name);
        std::string err = "[Semantic Error] Line " + std::to_string(stmt->line) +
                          ", Column " + std::to_string(stmt->column) +
                          ": Redefinition of variable '" + stmt->name + "'";
        if (prev) {
            err += " (previously declared at line " + std::to_string(prev->lineDeclared) + ")";
        }
        errors.push_back(err);
    }
}

void SemanticAnalyzer::analyzeAssign(const AssignStmtAST* stmt) {
    if (!symbolTable.isDeclared(stmt->name)) {
        std::string err = "[Semantic Error] Line " + std::to_string(stmt->line) +
                          ", Column " + std::to_string(stmt->column) +
                          ": Assignment to undeclared variable '" + stmt->name + "'";
        errors.push_back(err);
    } else {
        symbolTable.markInitialized(stmt->name);
    }

    if (stmt->expr) {
        analyzeExpression(stmt->expr.get());
    }
}

void SemanticAnalyzer::analyzePrint(const PrintStmtAST* stmt) {
    if (stmt->expr) {
        analyzeExpression(stmt->expr.get());
    }
}

void SemanticAnalyzer::analyzeExpression(const ExprAST* expr) {
    if (!expr) return;

    if (auto var = dynamic_cast<const VariableExprAST*>(expr)) {
        if (!symbolTable.isDeclared(var->name)) {
            std::string err = "[Semantic Error] Line " + std::to_string(var->line) +
                              ", Column " + std::to_string(var->column) +
                              ": Use of undeclared variable '" + var->name + "'";
            errors.push_back(err);
        }
    } else if (auto binary = dynamic_cast<const BinaryExprAST*>(expr)) {
        analyzeExpression(binary->lhs.get());
        analyzeExpression(binary->rhs.get());
    }
    // NumberExprAST does not require semantic validation
}
