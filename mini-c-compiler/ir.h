#ifndef MINI_C_IR_H
#define MINI_C_IR_H

#include "ast.h"
#include <string>
#include <vector>
#include <iostream>

// ============================================================================
// Three-Address Code (TAC) Instruction
// ============================================================================
enum class IROp {
    ASSIGN,     // dst = src1
    BINARY,     // dst = src1 op src2
    PRINT       // PRINT src1
};

struct IRInstruction {
    IROp op;
    std::string dst;
    std::string src1;
    std::string src2;
    char binaryOp = '\0';

    std::string toString() const;
};

// ============================================================================
// IR Generator
// ============================================================================
class IRGenerator {
public:
    IRGenerator() = default;

    // Generates Three-Address Code from the AST
    std::vector<IRInstruction> generate(const ProgramAST& program);

    // Prints the generated IR to a stream
    void print(std::ostream& os) const;

    // Returns the complete IR as a formatted string
    std::string getIRString() const;

    const std::vector<IRInstruction>& getInstructions() const { return instructions; }

private:
    int tempCounter = 0;
    std::vector<IRInstruction> instructions;

    // Allocates the next virtual register/temporary (e.g. t0, t1, t2...)
    std::string newTemp();

    // Recursively generates IR for expressions and returns the operand name
    std::string generateExpr(const ExprAST* expr);
};

#endif // MINI_C_IR_H
