#ifndef MINI_C_PARSER_H
#define MINI_C_PARSER_H

#include "lexer.h"
#include "ast.h"
#include <vector>
#include <string>
#include <memory>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parses the token stream into an Abstract Syntax Tree
    std::unique_ptr<ProgramAST> parse();

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    std::vector<Token> tokens;
    size_t current = 0;
    std::vector<std::string> errors;

    // Helper token stream navigation methods
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& expect(TokenType type, const std::string& errorMessage);

    void error(const Token& token, const std::string& message);
    void synchronize();

    // Grammar parsing methods
    std::unique_ptr<StmtAST> parseStatement();
    std::unique_ptr<StmtAST> parseVarDecl();
    std::unique_ptr<StmtAST> parseAssignment();
    std::unique_ptr<StmtAST> parsePrint();

    std::unique_ptr<ExprAST> parseExpression();
    std::unique_ptr<ExprAST> parseTerm();
    std::unique_ptr<ExprAST> parseFactor();
    std::unique_ptr<ExprAST> parsePrimary();
};

#endif // MINI_C_PARSER_H
