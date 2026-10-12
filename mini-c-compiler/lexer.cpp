#include "lexer.h"
#include <cctype>

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

Lexer::Lexer(std::string source) : source(std::move(source)) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[cursor];
}

char Lexer::peekNext() const {
    if (cursor + 1 >= source.size()) return '\0';
    return source[cursor + 1];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source[cursor++];
    column++;
    return c;
}

bool Lexer::isAtEnd() const {
    return cursor >= source.size();
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            advance();
            line++;
            column = 1;
        } else if (c == '/' && peekNext() == '/') {
            // Single-line comment: skip until newline
            advance(); // '/'
            advance(); // '/'
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::scanIdentifierOrKeyword(int startCol) {
    size_t startPos = cursor;
    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }

    std::string text = source.substr(startPos, cursor - startPos);

    if (text == "int") {
        return Token(TokenType::KEYWORD_INT, text, line, startCol);
    }
    if (text == "print") {
        return Token(TokenType::KEYWORD_PRINT, text, line, startCol);
    }
    return Token(TokenType::IDENTIFIER, text, line, startCol);
}

Token Lexer::scanNumber(int startCol) {
    size_t startPos = cursor;
    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }
    std::string text = source.substr(startPos, cursor - startPos);
    return Token(TokenType::INT_LITERAL, text, line, startCol);
}

Token Lexer::scanToken() {
    skipWhitespaceAndComments();

    if (isAtEnd()) {
        return Token(TokenType::END_OF_FILE, "", line, column);
    }

    int startCol = column;
    char c = peek();

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return scanIdentifierOrKeyword(startCol);
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        return scanNumber(startCol);
    }

    // Single character tokens
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
            std::string err = "[Lexer Error] Line " + std::to_string(line) +
                              ", Column " + std::to_string(startCol) +
                              ": Invalid character '" + std::string(1, c) + "'";
            errors.push_back(err);
            return Token(TokenType::UNKNOWN, std::string(1, c), line, startCol);
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token token = scanToken();
        tokens.push_back(token);
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    return tokens;
}
