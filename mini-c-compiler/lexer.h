#ifndef MINI_C_LEXER_H
#define MINI_C_LEXER_H

#include <string>
#include <vector>
#include <iostream>

// ============================================================================
// Token Definitions
// ============================================================================
enum class TokenType {
    // Keywords
    KEYWORD_INT,    // "int"
    KEYWORD_PRINT,  // "print"

    // Identifiers & Literals
    IDENTIFIER,     // variable names e.g., "a", "count"
    INT_LITERAL,    // integer constants e.g., "10", "42"

    // Operators
    PLUS,           // "+"
    MINUS,          // "-"
    STAR,           // "*"
    SLASH,          // "/"
    ASSIGN,         // "="

    // Delimiters
    LPAREN,         // "("
    RPAREN,         // ")"
    SEMICOLON,      // ";"

    // Special
    END_OF_FILE,    // End of source stream
    UNKNOWN         // Unrecognized character / lexical error
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType type, std::string lexeme, int line, int column)
        : type(type), lexeme(std::move(lexeme)), line(line), column(column) {}
};

// Helper to convert TokenType to human-readable string
std::string tokenTypeToString(TokenType type);

// ============================================================================
// Lexer Class
// ============================================================================
class Lexer {
public:
    explicit Lexer(std::string source);

    // Scans all tokens until END_OF_FILE
    std::vector<Token> tokenize();

    // Check if any lexical errors were encountered
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    std::string source;
    size_t cursor = 0;
    int line = 1;
    int column = 1;
    std::vector<std::string> errors;

    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;

    void skipWhitespaceAndComments();
    Token scanToken();
    Token scanIdentifierOrKeyword(int startCol);
    Token scanNumber(int startCol);
};

#endif // MINI_C_LEXER_H
