#include "ir.h"
#include <sstream>

// ============================================================================
// IRInstruction Implementation
// ============================================================================
std::string IRInstruction::toString() const {
    switch (op) {
        case IROp::ASSIGN:
            return dst + " = " + src1;
        case IROp::BINARY:
            return dst + " = " + src1 + " " + binaryOp + " " + src2;
        case IROp::PRINT:
            return "PRINT " + src1;
    }
    return "";
}

// ============================================================================
// IRGenerator Implementation
// ============================================================================
std::string IRGenerator::newTemp() {
    return "t" + std::to_string(tempCounter++);
}

std::string IRGenerator::generateExpr(const ExprAST* expr) {
    if (!expr) return "";

    if (auto num = dynamic_cast<const NumberExprAST*>(expr)) {
        return std::to_string(num->value);
    }

    if (auto var = dynamic_cast<const VariableExprAST*>(expr)) {
        return var->name;
    }

    if (auto bin = dynamic_cast<const BinaryExprAST*>(expr)) {
        std::string left = generateExpr(bin->lhs.get());
        std::string right = generateExpr(bin->rhs.get());
        std::string temp = newTemp();

        IRInstruction inst;
        inst.op = IROp::BINARY;
        inst.dst = temp;
        inst.src1 = left;
        inst.src2 = right;
        inst.binaryOp = bin->op;
        instructions.push_back(inst);

        return temp;
    }

    return "";
}

std::vector<IRInstruction> IRGenerator::generate(const ProgramAST& program) {
    instructions.clear();
    tempCounter = 0;

    for (const auto& stmt : program.statements) {
        if (!stmt) continue;

        if (auto varDecl = dynamic_cast<const VarDeclStmtAST*>(stmt.get())) {
            if (varDecl->initExpr) {
                std::string val = generateExpr(varDecl->initExpr.get());
                IRInstruction inst;
                inst.op = IROp::ASSIGN;
                inst.dst = varDecl->name;
                inst.src1 = val;
                instructions.push_back(inst);
            }
        } else if (auto assign = dynamic_cast<const AssignStmtAST*>(stmt.get())) {
            if (assign->expr) {
                std::string val = generateExpr(assign->expr.get());
                IRInstruction inst;
                inst.op = IROp::ASSIGN;
                inst.dst = assign->name;
                inst.src1 = val;
                instructions.push_back(inst);
            }
        } else if (auto print = dynamic_cast<const PrintStmtAST*>(stmt.get())) {
            if (print->expr) {
                std::string val = generateExpr(print->expr.get());
                IRInstruction inst;
                inst.op = IROp::PRINT;
                inst.src1 = val;
                instructions.push_back(inst);
            }
        }
    }

    return instructions;
}

void IRGenerator::print(std::ostream& os) const {
    for (const auto& inst : instructions) {
        os << inst.toString() << "\n";
    }
}

std::string IRGenerator::getIRString() const {
    std::ostringstream oss;
    print(oss);
    return oss.str();
}
