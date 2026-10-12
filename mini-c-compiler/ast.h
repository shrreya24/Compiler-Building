#ifndef MINI_C_AST_H
#define MINI_C_AST_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// ============================================================================
// Base AST Node
// ============================================================================
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void print(std::ostream& os, int indent = 0) const = 0;
};

// ============================================================================
// Expression AST Nodes
// ============================================================================
class ExprAST : public ASTNode {
public:
    int line = 0;
    int column = 0;

    virtual ~ExprAST() = default;
    virtual bool isConstant() const { return false; }
    virtual int getConstantValue() const { return 0; }
};

class NumberExprAST : public ExprAST {
public:
    int value;

    NumberExprAST(int val, int line, int col) : value(val) {
        this->line = line;
        this->column = col;
    }

    bool isConstant() const override { return true; }
    int getConstantValue() const override { return value; }

    void print(std::ostream& os, int indent = 0) const override {
        std::string spaces(indent, ' ');
        os << spaces << "Number: " << value << "\n";
    }
};

class VariableExprAST : public ExprAST {
public:
    std::string name;

    VariableExprAST(std::string name, int line, int col) : name(std::move(name)) {
        this->line = line;
        this->column = col;
    }

    void print(std::ostream& os, int indent = 0) const override {
        std::string spaces(indent, ' ');
        os << spaces << "Variable: " << name << "\n";
    }
};

class BinaryExprAST : public ExprAST {
public:
    char op; // '+', '-', '*', '/'
    std::unique_ptr<ExprAST> lhs;
    std::unique_ptr<ExprAST> rhs;

    BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs, int line, int col)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {
        this->line = line;
        this->column = col;
    }

    void print(std::ostream& os, int indent = 0) const override {
        std::string spaces(indent, ' ');
        os << spaces << "BinaryOp (" << op << ")\n";
        if (lhs) lhs->print(os, indent + 2);
        if (rhs) rhs->print(os, indent + 2);
    }
};

// ============================================================================
// Statement AST Nodes
// ============================================================================
class StmtAST : public ASTNode {
public:
    int line = 0;
    int column = 0;
    virtual ~StmtAST() = default;
};

class VarDeclStmtAST : public StmtAST {
public:
    std::string type; // "int"
    std::string name;
    std::unique_ptr<ExprAST> initExpr; // Can be nullptr if uninitialized

    VarDeclStmtAST(std::string type, std::string name, std::unique_ptr<ExprAST> initExpr, int line, int col)
        : type(std::move(type)), name(std::move(name)), initExpr(std::move(initExpr)) {
        this->line = line;
        this->column = col;
    }

    void print(std::ostream& os, int indent = 0) const override {
        std::string spaces(indent, ' ');
        os << spaces << "VarDecl: " << type << " " << name;
        if (initExpr) {
            os << " =\n";
            initExpr->print(os, indent + 2);
        } else {
            os << " (uninitialized)\n";
        }
    }
};

class AssignStmtAST : public StmtAST {
public:
    std::string name;
    std::unique_ptr<ExprAST> expr;

    AssignStmtAST(std::string name, std::unique_ptr<ExprAST> expr, int line, int col)
        : name(std::move(name)), expr(std::move(expr)) {
        this->line = line;
        this->column = col;
    }

    void print(std::ostream& os, int indent = 0) const override {
        std::string spaces(indent, ' ');
        os << spaces << "Assign: " << name << " =\n";
        if (expr) expr->print(os, indent + 2);
    }
};

class PrintStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> expr;

    PrintStmtAST(std::unique_ptr<ExprAST> expr, int line, int col)
        : expr(std::move(expr)) {
        this->line = line;
        this->column = col;
    }

    void print(std::ostream& os, int indent = 0) const override {
        std::string spaces(indent, ' ');
        os << spaces << "Print:\n";
        if (expr) expr->print(os, indent + 2);
    }
};

// ============================================================================
// Program AST (Root Node)
// ============================================================================
class ProgramAST : public ASTNode {
public:
    std::vector<std::unique_ptr<StmtAST>> statements;

    void addStatement(std::unique_ptr<StmtAST> stmt) {
        statements.push_back(std::move(stmt));
    }

    void print(std::ostream& os, int indent = 0) const override {
        std::string spaces(indent, ' ');
        os << spaces << "Program:\n";
        for (const auto& stmt : statements) {
            if (stmt) stmt->print(os, indent + 2);
        }
    }
};

// ============================================================================
// AST Constant Folding Optimizer
// ============================================================================
// Folds constant expressions at compile-time (e.g. 2 + 3 * 4 -> 14).
// Safely ignores unsafe operations such as division by zero.
inline std::unique_ptr<ExprAST> foldExpression(std::unique_ptr<ExprAST> expr) {
    if (!expr) return nullptr;

    auto binary = dynamic_cast<BinaryExprAST*>(expr.get());
    if (binary) {
        binary->lhs = foldExpression(std::move(binary->lhs));
        binary->rhs = foldExpression(std::move(binary->rhs));

        if (binary->lhs && binary->rhs && binary->lhs->isConstant() && binary->rhs->isConstant()) {
            int leftVal = binary->lhs->getConstantValue();
            int rightVal = binary->rhs->getConstantValue();
            int result = 0;

            switch (binary->op) {
                case '+':
                    result = leftVal + rightVal;
                    return std::make_unique<NumberExprAST>(result, binary->line, binary->column);
                case '-':
                    result = leftVal - rightVal;
                    return std::make_unique<NumberExprAST>(result, binary->line, binary->column);
                case '*':
                    result = leftVal * rightVal;
                    return std::make_unique<NumberExprAST>(result, binary->line, binary->column);
                case '/':
                    // Safety check: Do NOT fold division by zero!
                    if (rightVal == 0) {
                        return expr; // Keep unfolding to prevent runtime compiler crash
                    }
                    result = leftVal / rightVal;
                    return std::make_unique<NumberExprAST>(result, binary->line, binary->column);
                default:
                    break;
            }
        }
    }
    return expr;
}

inline void optimizeProgram(ProgramAST& program) {
    for (auto& stmt : program.statements) {
        if (auto varDecl = dynamic_cast<VarDeclStmtAST*>(stmt.get())) {
            varDecl->initExpr = foldExpression(std::move(varDecl->initExpr));
        } else if (auto assign = dynamic_cast<AssignStmtAST*>(stmt.get())) {
            assign->expr = foldExpression(std::move(assign->expr));
        } else if (auto print = dynamic_cast<PrintStmtAST*>(stmt.get())) {
            print->expr = foldExpression(std::move(print->expr));
        }
    }
}

#endif // MINI_C_AST_H
