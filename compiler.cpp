/**
 * ============================================================================
 * Mini C Compiler (C++17) - All-in-One Implementation
 * ============================================================================
 * Target: System Software Intern Role (NVIDIA)
 * 
 * Pipeline Architecture:
 *   Source Code (.mc)
 *          │
 *          ▼
 *    1. Lexer         --> Token stream (Keywords, Identifiers, Literals, Operators)
 *          │
 *          ▼
 *    2. Parser        --> Recursive-Descent LL(1) parsing into Abstract Syntax Tree (AST)
 *          │
 *          ▼
 *    3. Semantics     --> Symbol Table, Scope checks (undeclared, duplicate declarations)
 *          │
 *          ▼
 *    4. Optimizer     --> AST Constant Folding (e.g., 2 + 3 * 4 -> 14, safe div-by-zero)
 *          │
 *          ▼
 *    5. IR Generator  --> Three-Address Code (TAC) emission (t0 = b * 2, t1 = a + t0...)
 * ============================================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cctype>

// ============================================================================
// STAGE 1: LEXICAL ANALYSIS (LEXER)
// ============================================================================
enum class TokenType {
    KEYWORD_INT,    // "int"
    KEYWORD_PRINT,  // "print"
    IDENTIFIER,     // e.g. "a", "counter"
    INT_LITERAL,    // e.g. "10", "42"
    PLUS,           // "+"
    MINUS,          // "-"
    STAR,           // "*"
    SLASH,          // "/"
    ASSIGN,         // "="
    LPAREN,         // "("
    RPAREN,         // ")"
    SEMICOLON,      // ";"
    END_OF_FILE,    // EOF
    UNKNOWN         // Invalid character
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType type, std::string lexeme, int line, int col)
        : type(type), lexeme(std::move(lexeme)), line(line), column(col) {}
};

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD_INT:   return "KEYWORD_INT";
        case TokenType::KEYWORD_PRINT: return "KEYWORD_PRINT";
        case TokenType::IDENTIFIER:    return "IDENTIFIER";
        case TokenType::INT_LITERAL:   return "INT_LITERAL";
        case TokenType::PLUS:          return "PLUS";
        case TokenType::MINUS:         return "MINUS";
        case TokenType::STAR:          return "STAR";
        case TokenType::SLASH:         return "SLASH";
        case TokenType::ASSIGN:        return "ASSIGN";
        case TokenType::LPAREN:        return "LPAREN";
        case TokenType::RPAREN:        return "RPAREN";
        case TokenType::SEMICOLON:     return "SEMICOLON";
        case TokenType::END_OF_FILE:   return "EOF";
        case TokenType::UNKNOWN:       return "UNKNOWN";
    }
    return "UNKNOWN";
}

class Lexer {
public:
    explicit Lexer(std::string src) : source(std::move(src)) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (true) {
            Token tok = nextToken();
            tokens.push_back(tok);
            if (tok.type == TokenType::END_OF_FILE) break;
        }
        return tokens;
    }

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    std::string source;
    size_t cursor = 0;
    int line = 1;
    int column = 1;
    std::vector<std::string> errors;

    bool isAtEnd() const { return cursor >= source.size(); }
    char peek() const { return isAtEnd() ? '\0' : source[cursor]; }
    char peekNext() const { return (cursor + 1 >= source.size()) ? '\0' : source[cursor + 1]; }
    char advance() { char c = source[cursor++]; column++; return c; }

    void skipWhitespaceAndComments() {
        while (!isAtEnd()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r') {
                advance();
            } else if (c == '\n') {
                advance();
                line++;
                column = 1;
            } else if (c == '/' && peekNext() == '/') {
                advance(); advance();
                while (!isAtEnd() && peek() != '\n') advance();
            } else {
                break;
            }
        }
    }

    Token nextToken() {
        skipWhitespaceAndComments();
        if (isAtEnd()) return Token(TokenType::END_OF_FILE, "", line, column);

        int startCol = column;
        char c = peek();

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t startPos = cursor;
            while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
                advance();
            }
            std::string text = source.substr(startPos, cursor - startPos);
            if (text == "int")   return Token(TokenType::KEYWORD_INT, text, line, startCol);
            if (text == "print") return Token(TokenType::KEYWORD_PRINT, text, line, startCol);
            return Token(TokenType::IDENTIFIER, text, line, startCol);
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            size_t startPos = cursor;
            while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
            std::string text = source.substr(startPos, cursor - startPos);
            return Token(TokenType::INT_LITERAL, text, line, startCol);
        }

        advance();
        switch (c) {
            case '+': return Token(TokenType::PLUS, "+", line, startCol);
            case '-': return Token(TokenType::MINUS, "-", line, startCol);
            case '*': return Token(TokenType::STAR, "*", line, startCol);
            case '/': return Token(TokenType::SLASH, "/", line, startCol);
            case '=': return Token(TokenType::ASSIGN, "=", line, startCol);
            case '(': return Token(TokenType::LPAREN, "(", line, startCol);
            case ')': return Token(TokenType::RPAREN, ")", line, startCol);
            case ';': return Token(TokenType::SEMICOLON, ";", line, startCol);
            default: {
                errors.push_back("[Lexer Error] Line " + std::to_string(line) +
                                 ", Column " + std::to_string(startCol) +
                                 ": Invalid character '" + std::string(1, c) + "'");
                return Token(TokenType::UNKNOWN, std::string(1, c), line, startCol);
            }
        }
    }
};

// ============================================================================
// STAGE 2: ABSTRACT SYNTAX TREE (AST) & CONSTANT FOLDING
// ============================================================================
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

class ExprAST : public ASTNode {
public:
    int line = 0, column = 0;
    virtual ~ExprAST() = default;
    virtual bool isConstant() const { return false; }
    virtual int getConstantValue() const { return 0; }
};

class NumberExprAST : public ExprAST {
public:
    int value;
    NumberExprAST(int val, int l, int c) : value(val) { line = l; column = c; }
    bool isConstant() const override { return true; }
    int getConstantValue() const override { return value; }
};

class VariableExprAST : public ExprAST {
public:
    std::string name;
    VariableExprAST(std::string n, int l, int c) : name(std::move(n)) { line = l; column = c; }
};

class BinaryExprAST : public ExprAST {
public:
    char op;
    std::unique_ptr<ExprAST> lhs;
    std::unique_ptr<ExprAST> rhs;
    BinaryExprAST(char op, std::unique_ptr<ExprAST> l, std::unique_ptr<ExprAST> r, int line, int col)
        : op(op), lhs(std::move(l)), rhs(std::move(r)) { this->line = line; this->column = col; }
};

class StmtAST : public ASTNode {
public:
    int line = 0, column = 0;
    virtual ~StmtAST() = default;
};

class VarDeclStmtAST : public StmtAST {
public:
    std::string type, name;
    std::unique_ptr<ExprAST> initExpr;
    VarDeclStmtAST(std::string t, std::string n, std::unique_ptr<ExprAST> init, int l, int c)
        : type(std::move(t)), name(std::move(n)), initExpr(std::move(init)) { line = l; column = c; }
};

class AssignStmtAST : public StmtAST {
public:
    std::string name;
    std::unique_ptr<ExprAST> expr;
    AssignStmtAST(std::string n, std::unique_ptr<ExprAST> e, int l, int c)
        : name(std::move(n)), expr(std::move(e)) { line = l; column = c; }
};

class PrintStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> expr;
    PrintStmtAST(std::unique_ptr<ExprAST> e, int l, int c)
        : expr(std::move(e)) { line = l; column = c; }
};

class ProgramAST : public ASTNode {
public:
    std::vector<std::unique_ptr<StmtAST>> statements;
};

// Constant Folding Optimizer
inline std::unique_ptr<ExprAST> foldExpression(std::unique_ptr<ExprAST> expr) {
    if (!expr) return nullptr;
    auto binary = dynamic_cast<BinaryExprAST*>(expr.get());
    if (binary) {
        binary->lhs = foldExpression(std::move(binary->lhs));
        binary->rhs = foldExpression(std::move(binary->rhs));
        if (binary->lhs && binary->rhs && binary->lhs->isConstant() && binary->rhs->isConstant()) {
            int l = binary->lhs->getConstantValue();
            int r = binary->rhs->getConstantValue();
            switch (binary->op) {
                case '+': return std::make_unique<NumberExprAST>(l + r, binary->line, binary->column);
                case '-': return std::make_unique<NumberExprAST>(l - r, binary->line, binary->column);
                case '*': return std::make_unique<NumberExprAST>(l * r, binary->line, binary->column);
                case '/':
                    if (r == 0) return expr; // Safety: Never fold division by zero!
                    return std::make_unique<NumberExprAST>(l / r, binary->line, binary->column);
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

// ============================================================================
// STAGE 3: SYNTAX ANALYSIS (RECURSIVE-DESCENT PARSER)
// ============================================================================
class Parser {
public:
    explicit Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}

    std::unique_ptr<ProgramAST> parse() {
        auto prog = std::make_unique<ProgramAST>();
        while (!isAtEnd()) {
            auto stmt = parseStatement();
            if (stmt) prog->statements.push_back(std::move(stmt));
            else synchronize();
        }
        return prog;
    }

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    std::vector<Token> tokens;
    size_t current = 0;
    std::vector<std::string> errors;

    bool isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }
    const Token& peek() const { return tokens[current]; }
    const Token& previous() const { return tokens[current == 0 ? 0 : current - 1]; }
    const Token& advance() { if (!isAtEnd()) current++; return previous(); }
    bool check(TokenType t) const { return isAtEnd() ? false : peek().type == t; }
    bool match(TokenType t) { if (check(t)) { advance(); return true; } return false; }

    void error(const Token& tok, const std::string& msg) {
        std::string err = "[Parser Error] Line " + std::to_string(tok.line) +
                          ", Column " + std::to_string(tok.column) + ": " + msg;
        if (tok.type != TokenType::END_OF_FILE && !tok.lexeme.empty()) {
            err += " (got '" + tok.lexeme + "')";
        }
        errors.push_back(err);
    }

    void synchronize() {
        advance();
        while (!isAtEnd()) {
            if (previous().type == TokenType::SEMICOLON) return;
            if (peek().type == TokenType::KEYWORD_INT || peek().type == TokenType::KEYWORD_PRINT) return;
            advance();
        }
    }

    std::unique_ptr<StmtAST> parseStatement() {
        if (check(TokenType::KEYWORD_INT))   return parseVarDecl();
        if (check(TokenType::KEYWORD_PRINT)) return parsePrint();
        if (check(TokenType::IDENTIFIER))    return parseAssignment();
        error(peek(), "Expected statement (variable declaration, assignment, or print)");
        return nullptr;
    }

    std::unique_ptr<StmtAST> parseVarDecl() {
        Token typeTok = advance();
        if (!check(TokenType::IDENTIFIER)) {
            error(peek(), "Expected identifier after 'int'");
            return nullptr;
        }
        Token idTok = advance();
        std::unique_ptr<ExprAST> init = nullptr;
        if (match(TokenType::ASSIGN)) {
            init = parseExpression();
            if (!init) return nullptr;
        }
        if (!check(TokenType::SEMICOLON)) {
            error(peek(), "Expected ';' after variable declaration");
            return nullptr;
        }
        advance();
        return std::make_unique<VarDeclStmtAST>(typeTok.lexeme, idTok.lexeme, std::move(init), typeTok.line, typeTok.column);
    }

    std::unique_ptr<StmtAST> parseAssignment() {
        Token idTok = advance();
        if (!match(TokenType::ASSIGN)) {
            error(peek(), "Expected '=' after identifier in assignment");
            return nullptr;
        }
        auto expr = parseExpression();
        if (!expr) return nullptr;
        if (!check(TokenType::SEMICOLON)) {
            error(peek(), "Expected ';' after assignment statement");
            return nullptr;
        }
        advance();
        return std::make_unique<AssignStmtAST>(idTok.lexeme, std::move(expr), idTok.line, idTok.column);
    }

    std::unique_ptr<StmtAST> parsePrint() {
        Token printTok = advance();
        if (!match(TokenType::LPAREN)) {
            error(peek(), "Expected '(' after 'print'");
            return nullptr;
        }
        auto expr = parseExpression();
        if (!expr) return nullptr;
        if (!match(TokenType::RPAREN)) {
            error(peek(), "Expected ')' after print expression");
            return nullptr;
        }
        if (!check(TokenType::SEMICOLON)) {
            error(peek(), "Expected ';' after print statement");
            return nullptr;
        }
        advance();
        return std::make_unique<PrintStmtAST>(std::move(expr), printTok.line, printTok.column);
    }

    std::unique_ptr<ExprAST> parseExpression() {
        auto expr = parseTerm();
        if (!expr) return nullptr;
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            Token op = advance();
            auto rhs = parseTerm();
            if (!rhs) return nullptr;
            expr = std::make_unique<BinaryExprAST>(op.lexeme[0], std::move(expr), std::move(rhs), op.line, op.column);
        }
        return expr;
    }

    std::unique_ptr<ExprAST> parseTerm() {
        auto expr = parseFactor();
        if (!expr) return nullptr;
        while (check(TokenType::STAR) || check(TokenType::SLASH)) {
            Token op = advance();
            auto rhs = parseFactor();
            if (!rhs) return nullptr;
            expr = std::make_unique<BinaryExprAST>(op.lexeme[0], std::move(expr), std::move(rhs), op.line, op.column);
        }
        return expr;
    }

    std::unique_ptr<ExprAST> parseFactor() {
        if (match(TokenType::MINUS)) {
            Token op = previous();
            auto primary = parsePrimary();
            if (!primary) return nullptr;
            auto zero = std::make_unique<NumberExprAST>(0, op.line, op.column);
            return std::make_unique<BinaryExprAST>('-', std::move(zero), std::move(primary), op.line, op.column);
        }
        if (match(TokenType::PLUS)) return parsePrimary();
        return parsePrimary();
    }

    std::unique_ptr<ExprAST> parsePrimary() {
        if (match(TokenType::INT_LITERAL)) {
            Token t = previous();
            return std::make_unique<NumberExprAST>(std::stoi(t.lexeme), t.line, t.column);
        }
        if (match(TokenType::IDENTIFIER)) {
            Token t = previous();
            return std::make_unique<VariableExprAST>(t.lexeme, t.line, t.column);
        }
        if (match(TokenType::LPAREN)) {
            auto e = parseExpression();
            if (!e) return nullptr;
            if (!match(TokenType::RPAREN)) {
                error(peek(), "Expected ')' after grouped expression");
                return nullptr;
            }
            return e;
        }
        error(peek(), "Expected expression (number, identifier, or '(')");
        return nullptr;
    }
};

// ============================================================================
// STAGE 4: SEMANTIC ANALYSIS & SYMBOL TABLE
// ============================================================================
struct Symbol {
    std::string name;
    std::string type;
    int line;
};

class SemanticAnalyzer {
public:
    bool analyze(const ProgramAST& prog) {
        errors.clear();
        symbols.clear();
        for (const auto& stmt : prog.statements) {
            if (auto varDecl = dynamic_cast<const VarDeclStmtAST*>(stmt.get())) {
                if (varDecl->initExpr) checkExpr(varDecl->initExpr.get());
                if (symbols.find(varDecl->name) != symbols.end()) {
                    errors.push_back("[Semantic Error] Line " + std::to_string(varDecl->line) +
                                     ", Column " + std::to_string(varDecl->column) +
                                     ": Redefinition of variable '" + varDecl->name +
                                     "' (previously declared at line " + std::to_string(symbols[varDecl->name].line) + ")");
                } else {
                    symbols[varDecl->name] = {varDecl->name, varDecl->type, varDecl->line};
                }
            } else if (auto assign = dynamic_cast<const AssignStmtAST*>(stmt.get())) {
                if (symbols.find(assign->name) == symbols.end()) {
                    errors.push_back("[Semantic Error] Line " + std::to_string(assign->line) +
                                     ", Column " + std::to_string(assign->column) +
                                     ": Assignment to undeclared variable '" + assign->name + "'");
                }
                if (assign->expr) checkExpr(assign->expr.get());
            } else if (auto print = dynamic_cast<const PrintStmtAST*>(stmt.get())) {
                if (print->expr) checkExpr(print->expr.get());
            }
        }
        return errors.empty();
    }

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    std::unordered_map<std::string, Symbol> symbols;
    std::vector<std::string> errors;

    void checkExpr(const ExprAST* expr) {
        if (!expr) return;
        if (auto var = dynamic_cast<const VariableExprAST*>(expr)) {
            if (symbols.find(var->name) == symbols.end()) {
                errors.push_back("[Semantic Error] Line " + std::to_string(var->line) +
                                 ", Column " + std::to_string(var->column) +
                                 ": Use of undeclared variable '" + var->name + "'");
            }
        } else if (auto bin = dynamic_cast<const BinaryExprAST*>(expr)) {
            checkExpr(bin->lhs.get());
            checkExpr(bin->rhs.get());
        }
    }
};

// ============================================================================
// STAGE 5: INTERMEDIATE REPRESENTATION (THREE-ADDRESS CODE)
// ============================================================================
class IRGenerator {
public:
    std::vector<std::string> instructions;

    void generate(const ProgramAST& prog) {
        instructions.clear();
        tempCount = 0;
        for (const auto& stmt : prog.statements) {
            if (auto varDecl = dynamic_cast<const VarDeclStmtAST*>(stmt.get())) {
                if (varDecl->initExpr) {
                    std::string val = genExpr(varDecl->initExpr.get());
                    instructions.push_back(varDecl->name + " = " + val);
                }
            } else if (auto assign = dynamic_cast<const AssignStmtAST*>(stmt.get())) {
                if (assign->expr) {
                    std::string val = genExpr(assign->expr.get());
                    instructions.push_back(assign->name + " = " + val);
                }
            } else if (auto print = dynamic_cast<const PrintStmtAST*>(stmt.get())) {
                if (print->expr) {
                    std::string val = genExpr(print->expr.get());
                    instructions.push_back("PRINT " + val);
                }
            }
        }
    }

    void print(std::ostream& os) const {
        for (const auto& line : instructions) {
            os << line << "\n";
        }
    }

private:
    int tempCount = 0;

    std::string newTemp() {
        return "t" + std::to_string(tempCount++);
    }

    std::string genExpr(const ExprAST* expr) {
        if (!expr) return "";
        if (auto num = dynamic_cast<const NumberExprAST*>(expr)) {
            return std::to_string(num->value);
        }
        if (auto var = dynamic_cast<const VariableExprAST*>(expr)) {
            return var->name;
        }
        if (auto bin = dynamic_cast<const BinaryExprAST*>(expr)) {
            std::string left = genExpr(bin->lhs.get());
            std::string right = genExpr(bin->rhs.get());
            std::string temp = newTemp();
            instructions.push_back(temp + " = " + left + " " + bin->op + " " + right);
            return temp;
        }
        return "";
    }
};

// ============================================================================
// MAIN COMPILER DRIVER
// ============================================================================
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <source_file.mc> [--no-opt]\n";
        return 1;
    }

    std::string filename = argv[1];
    bool optimize = true;
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--no-opt") optimize = false;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file '" << filename << "'\n";
        return 1;
    }

    std::stringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();

    // 1. Lexer
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    if (lexer.hasErrors()) {
        for (const auto& e : lexer.getErrors()) std::cerr << e << "\n";
        return 1;
    }

    // 2. Parser
    Parser parser(tokens);
    auto ast = parser.parse();
    if (parser.hasErrors()) {
        for (const auto& e : parser.getErrors()) std::cerr << e << "\n";
        return 1;
    }

    // 3. Semantics
    SemanticAnalyzer semantic;
    if (!semantic.analyze(*ast)) {
        for (const auto& e : semantic.getErrors()) std::cerr << e << "\n";
        return 1;
    }

    // 4. Optimizer
    if (optimize) {
        optimizeProgram(*ast);
    }

    // 5. IR Emission (Three-Address Code)
    IRGenerator irGen;
    irGen.generate(*ast);
    irGen.print(std::cout);

    return 0;
}
