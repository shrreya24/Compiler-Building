#include "parser.h"

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    if (current == 0) return tokens[0];
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        current++;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenType type, const std::string& errorMessage) {
    if (check(type)) {
        return advance();
    }
    error(peek(), errorMessage);
    return peek();
}

void Parser::error(const Token& token, const std::string& message) {
    std::string err = "[Parser Error] Line " + std::to_string(token.line) +
                      ", Column " + std::to_string(token.column) +
                      ": " + message;
    if (token.type != TokenType::END_OF_FILE && !token.lexeme.empty()) {
        err += " (got '" + token.lexeme + "')";
    }
    errors.push_back(err);
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;

        switch (peek().type) {
            case TokenType::KEYWORD_INT:
            case TokenType::KEYWORD_PRINT:
                return;
            default:
                break;
        }
        advance();
    }
}

std::unique_ptr<ProgramAST> Parser::parse() {
    auto program = std::make_unique<ProgramAST>();
    while (!isAtEnd()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                program->addStatement(std::move(stmt));
            } else {
                synchronize();
            }
        } catch (...) {
            synchronize();
        }
    }
    return program;
}

std::unique_ptr<StmtAST> Parser::parseStatement() {
    if (check(TokenType::KEYWORD_INT)) {
        return parseVarDecl();
    }
    if (check(TokenType::KEYWORD_PRINT)) {
        return parsePrint();
    }
    if (check(TokenType::IDENTIFIER)) {
        return parseAssignment();
    }

    error(peek(), "Expected statement (variable declaration, assignment, or print)");
    return nullptr;
}

std::unique_ptr<StmtAST> Parser::parseVarDecl() {
    Token typeToken = advance(); // "int"
    int line = typeToken.line;
    int col = typeToken.column;

    if (!check(TokenType::IDENTIFIER)) {
        error(peek(), "Expected identifier after 'int'");
        return nullptr;
    }
    Token idToken = advance();
    std::string varName = idToken.lexeme;

    std::unique_ptr<ExprAST> initExpr = nullptr;
    if (match(TokenType::ASSIGN)) {
        initExpr = parseExpression();
        if (!initExpr) return nullptr;
    }

    if (!check(TokenType::SEMICOLON)) {
        error(peek(), "Expected ';' after variable declaration");
        return nullptr;
    }
    advance(); // consume ';'

    return std::make_unique<VarDeclStmtAST>(typeToken.lexeme, varName, std::move(initExpr), line, col);
}

std::unique_ptr<StmtAST> Parser::parseAssignment() {
    Token idToken = advance(); // identifier
    int line = idToken.line;
    int col = idToken.column;

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
    advance(); // consume ';'

    return std::make_unique<AssignStmtAST>(idToken.lexeme, std::move(expr), line, col);
}

std::unique_ptr<StmtAST> Parser::parsePrint() {
    Token printToken = advance(); // "print"
    int line = printToken.line;
    int col = printToken.column;

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
    advance(); // consume ';'

    return std::make_unique<PrintStmtAST>(std::move(expr), line, col);
}

std::unique_ptr<ExprAST> Parser::parseExpression() {
    auto expr = parseTerm();
    if (!expr) return nullptr;

    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token opToken = advance();
        char op = opToken.lexeme[0];
        auto rhs = parseTerm();
        if (!rhs) return nullptr;
        expr = std::make_unique<BinaryExprAST>(op, std::move(expr), std::move(rhs), opToken.line, opToken.column);
    }
    return expr;
}

std::unique_ptr<ExprAST> Parser::parseTerm() {
    auto expr = parseFactor();
    if (!expr) return nullptr;

    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        Token opToken = advance();
        char op = opToken.lexeme[0];
        auto rhs = parseFactor();
        if (!rhs) return nullptr;
        expr = std::make_unique<BinaryExprAST>(op, std::move(expr), std::move(rhs), opToken.line, opToken.column);
    }
    return expr;
}

std::unique_ptr<ExprAST> Parser::parseFactor() {
    // Unary operators: + or -
    if (match(TokenType::MINUS)) {
        Token opToken = previous();
        auto primary = parsePrimary();
        if (!primary) return nullptr;
        auto zeroNode = std::make_unique<NumberExprAST>(0, opToken.line, opToken.column);
        return std::make_unique<BinaryExprAST>('-', std::move(zeroNode), std::move(primary), opToken.line, opToken.column);
    }
    if (match(TokenType::PLUS)) {
        return parsePrimary();
    }
    return parsePrimary();
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
    if (match(TokenType::INT_LITERAL)) {
        Token tok = previous();
        try {
            int val = std::stoi(tok.lexeme);
            return std::make_unique<NumberExprAST>(val, tok.line, tok.column);
        } catch (...) {
            error(tok, "Integer literal out of range: " + tok.lexeme);
            return nullptr;
        }
    }

    if (match(TokenType::IDENTIFIER)) {
        Token tok = previous();
        return std::make_unique<VariableExprAST>(tok.lexeme, tok.line, tok.column);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        if (!expr) return nullptr;
        if (!match(TokenType::RPAREN)) {
            error(peek(), "Expected ')' after grouped expression");
            return nullptr;
        }
        return expr;
    }

    error(peek(), "Expected expression (number, identifier, or '(')");
    return nullptr;
}
