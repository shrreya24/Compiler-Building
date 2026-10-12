import sys
import os
from enum import Enum, auto

# ============================================================================
# Token Definitions
# ============================================================================
class TokenType(Enum):
    KEYWORD_INT = auto()
    KEYWORD_PRINT = auto()
    IDENTIFIER = auto()
    INT_LITERAL = auto()
    PLUS = auto()
    MINUS = auto()
    STAR = auto()
    SLASH = auto()
    ASSIGN = auto()
    LPAREN = auto()
    RPAREN = auto()
    SEMICOLON = auto()
    EOF = auto()
    UNKNOWN = auto()

class Token:
    def __init__(self, typ, lexeme, line, col):
        self.type = typ
        self.lexeme = lexeme
        self.line = line
        self.column = col

    def __repr__(self):
        return f"Token({self.type.name}, '{self.lexeme}', L{self.line}:C{self.column})"

# ============================================================================
# Lexer
# ============================================================================
class Lexer:
    def __init__(self, source):
        self.source = source
        self.cursor = 0
        self.line = 1
        self.col = 1
        self.errors = []

    def is_at_end(self):
        return self.cursor >= len(self.source)

    def peek(self):
        return '\0' if self.is_at_end() else self.source[self.cursor]

    def peek_next(self):
        if self.cursor + 1 >= len(self.source):
            return '\0'
        return self.source[self.cursor + 1]

    def advance(self):
        c = self.peek()
        self.cursor += 1
        self.col += 1
        return c

    def skip_whitespace_and_comments(self):
        while not self.is_at_end():
            c = self.peek()
            if c in (' ', '\t', '\r'):
                self.advance()
            elif c == '\n':
                self.advance()
                self.line += 1
                self.col = 1
            elif c == '/' and self.peek_next() == '/':
                self.advance()
                self.advance()
                while not self.is_at_end() and self.peek() != '\n':
                    self.advance()
            else:
                break

    def tokenize(self):
        tokens = []
        while True:
            self.skip_whitespace_and_comments()
            if self.is_at_end():
                tokens.append(Token(TokenType.EOF, "", self.line, self.col))
                break

            start_col = self.col
            c = self.peek()

            if c.isalpha() or c == '_':
                start = self.cursor
                while not self.is_at_end() and (self.peek().isalnum() or self.peek() == '_'):
                    self.advance()
                text = self.source[start:self.cursor]
                if text == "int":
                    tokens.append(Token(TokenType.KEYWORD_INT, text, self.line, start_col))
                elif text == "print":
                    tokens.append(Token(TokenType.KEYWORD_PRINT, text, self.line, start_col))
                else:
                    tokens.append(Token(TokenType.IDENTIFIER, text, self.line, start_col))
            elif c.isdigit():
                start = self.cursor
                while not self.is_at_end() and self.peek().isdigit():
                    self.advance()
                text = self.source[start:self.cursor]
                tokens.append(Token(TokenType.INT_LITERAL, text, self.line, start_col))
            else:
                self.advance()
                mapping = {
                    '+': TokenType.PLUS,
                    '-': TokenType.MINUS,
                    '*': TokenType.STAR,
                    '/': TokenType.SLASH,
                    '=': TokenType.ASSIGN,
                    '(': TokenType.LPAREN,
                    ')': TokenType.RPAREN,
                    ';': TokenType.SEMICOLON
                }
                if c in mapping:
                    tokens.append(Token(mapping[c], c, self.line, start_col))
                else:
                    self.errors.append(f"[Lexer Error] Line {self.line}, Column {start_col}: Invalid character '{c}'")
                    tokens.append(Token(TokenType.UNKNOWN, c, self.line, start_col))
        return tokens

# ============================================================================
# AST Nodes
# ============================================================================
class ASTNode: pass

class ExprAST(ASTNode):
    def is_constant(self): return False
    def get_constant_value(self): return 0

class NumberExprAST(ExprAST):
    def __init__(self, val, line, col):
        self.val = val
        self.line = line
        self.col = col
    def is_constant(self): return True
    def get_constant_value(self): return self.val

class VariableExprAST(ExprAST):
    def __init__(self, name, line, col):
        self.name = name
        self.line = line
        self.col = col

class BinaryExprAST(ExprAST):
    def __init__(self, op, lhs, rhs, line, col):
        self.op = op
        self.lhs = lhs
        self.rhs = rhs
        self.line = line
        self.col = col

class StmtAST(ASTNode): pass

class VarDeclStmtAST(StmtAST):
    def __init__(self, typ, name, init_expr, line, col):
        self.type = typ
        self.name = name
        self.init_expr = init_expr
        self.line = line
        self.col = col

class AssignStmtAST(StmtAST):
    def __init__(self, name, expr, line, col):
        self.name = name
        self.expr = expr
        self.line = line
        self.col = col

class PrintStmtAST(StmtAST):
    def __init__(self, expr, line, col):
        self.expr = expr
        self.line = line
        self.col = col

class ProgramAST(ASTNode):
    def __init__(self):
        self.statements = []

# ============================================================================
# Parser
# ============================================================================
class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.current = 0
        self.errors = []

    def is_at_end(self):
        return self.peek().type == TokenType.EOF

    def peek(self):
        return self.tokens[self.current]

    def previous(self):
        return self.tokens[self.current - 1]

    def advance(self):
        if not self.is_at_end():
            self.current += 1
        return self.previous()

    def check(self, typ):
        return False if self.is_at_end() else self.peek().type == typ

    def match(self, typ):
        if self.check(typ):
            self.advance()
            return True
        return False

    def error(self, tok, msg):
        err = f"[Parser Error] Line {tok.line}, Column {tok.column}: {msg}"
        if tok.type != TokenType.EOF and tok.lexeme:
            err += f" (got '{tok.lexeme}')"
        self.errors.append(err)

    def synchronize(self):
        self.advance()
        while not self.is_at_end():
            if self.previous().type == TokenType.SEMICOLON:
                return
            if self.peek().type in (TokenType.KEYWORD_INT, TokenType.KEYWORD_PRINT):
                return
            self.advance()

    def parse(self):
        prog = ProgramAST()
        while not self.is_at_end():
            stmt = self.parse_statement()
            if stmt:
                prog.statements.append(stmt)
            else:
                self.synchronize()
        return prog

    def parse_statement(self):
        if self.check(TokenType.KEYWORD_INT):
            return self.parse_var_decl()
        if self.check(TokenType.KEYWORD_PRINT):
            return self.parse_print()
        if self.check(TokenType.IDENTIFIER):
            return self.parse_assignment()
        self.error(self.peek(), "Expected statement")
        return None

    def parse_var_decl(self):
        type_tok = self.advance()
        if not self.check(TokenType.IDENTIFIER):
            self.error(self.peek(), "Expected identifier after 'int'")
            return None
        id_tok = self.advance()
        init_expr = None
        if self.match(TokenType.ASSIGN):
            init_expr = self.parse_expression()
            if not init_expr: return None
        if not self.check(TokenType.SEMICOLON):
            self.error(self.peek(), "Expected ';' after variable declaration")
            return None
        self.advance()
        return VarDeclStmtAST(type_tok.lexeme, id_tok.lexeme, init_expr, type_tok.line, type_tok.column)

    def parse_assignment(self):
        id_tok = self.advance()
        if not self.match(TokenType.ASSIGN):
            self.error(self.peek(), "Expected '=' in assignment")
            return None
        expr = self.parse_expression()
        if not expr: return None
        if not self.check(TokenType.SEMICOLON):
            self.error(self.peek(), "Expected ';' after assignment statement")
            return None
        self.advance()
        return AssignStmtAST(id_tok.lexeme, expr, id_tok.line, id_tok.column)

    def parse_print(self):
        print_tok = self.advance()
        if not self.match(TokenType.LPAREN):
            self.error(self.peek(), "Expected '(' after 'print'")
            return None
        expr = self.parse_expression()
        if not expr: return None
        if not self.match(TokenType.RPAREN):
            self.error(self.peek(), "Expected ')' after print expression")
            return None
        if not self.check(TokenType.SEMICOLON):
            self.error(self.peek(), "Expected ';' after print statement")
            return None
        self.advance()
        return PrintStmtAST(expr, print_tok.line, print_tok.column)

    def parse_expression(self):
        expr = self.parse_term()
        if not expr: return None
        while self.check(TokenType.PLUS) or self.check(TokenType.MINUS):
            op_tok = self.advance()
            rhs = self.parse_term()
            if not rhs: return None
            expr = BinaryExprAST(op_tok.lexeme, expr, rhs, op_tok.line, op_tok.column)
        return expr

    def parse_term(self):
        expr = self.parse_factor()
        if not expr: return None
        while self.check(TokenType.STAR) or self.check(TokenType.SLASH):
            op_tok = self.advance()
            rhs = self.parse_factor()
            if not rhs: return None
            expr = BinaryExprAST(op_tok.lexeme, expr, rhs, op_tok.line, op_tok.column)
        return expr

    def parse_factor(self):
        if self.match(TokenType.MINUS):
            op_tok = self.previous()
            prim = self.parse_primary()
            if not prim: return None
            zero = NumberExprAST(0, op_tok.line, op_tok.column)
            return BinaryExprAST('-', zero, prim, op_tok.line, op_tok.column)
        if self.match(TokenType.PLUS):
            return self.parse_primary()
        return self.parse_primary()

    def parse_primary(self):
        if self.match(TokenType.INT_LITERAL):
            tok = self.previous()
            return NumberExprAST(int(tok.lexeme), tok.line, tok.column)
        if self.match(TokenType.IDENTIFIER):
            tok = self.previous()
            return VariableExprAST(tok.lexeme, tok.line, tok.column)
        if self.match(TokenType.LPAREN):
            expr = self.parse_expression()
            if not expr: return None
            if not self.match(TokenType.RPAREN):
                self.error(self.peek(), "Expected ')' after grouped expression")
                return None
            return expr
        self.error(self.peek(), "Expected expression")
        return None

# ============================================================================
# Semantic Analyzer
# ============================================================================
class SemanticAnalyzer:
    def __init__(self):
        self.table = {}
        self.errors = []

    def analyze(self, prog):
        for stmt in prog.statements:
            if isinstance(stmt, VarDeclStmtAST):
                if stmt.init_expr:
                    self.analyze_expr(stmt.init_expr)
                if stmt.name in self.table:
                    prev_line = self.table[stmt.name]['line']
                    self.errors.append(f"[Semantic Error] Line {stmt.line}, Column {stmt.col}: Redefinition of variable '{stmt.name}' (previously declared at line {prev_line})")
                else:
                    self.table[stmt.name] = {'type': stmt.type, 'line': stmt.line}
            elif isinstance(stmt, AssignStmtAST):
                if stmt.name not in self.table:
                    self.errors.append(f"[Semantic Error] Line {stmt.line}, Column {stmt.col}: Assignment to undeclared variable '{stmt.name}'")
                if stmt.expr:
                    self.analyze_expr(stmt.expr)
            elif isinstance(stmt, PrintStmtAST):
                if stmt.expr:
                    self.analyze_expr(stmt.expr)
        return len(self.errors) == 0

    def analyze_expr(self, expr):
        if isinstance(expr, VariableExprAST):
            if expr.name not in self.table:
                self.errors.append(f"[Semantic Error] Line {expr.line}, Column {expr.col}: Use of undeclared variable '{expr.name}'")
        elif isinstance(expr, BinaryExprAST):
            self.analyze_expr(expr.lhs)
            self.analyze_expr(expr.rhs)

# ============================================================================
# Constant Folding Optimizer
# ============================================================================
def fold_expr(expr):
    if not expr: return None
    if isinstance(expr, BinaryExprAST):
        expr.lhs = fold_expr(expr.lhs)
        expr.rhs = fold_expr(expr.rhs)
        if expr.lhs and expr.rhs and expr.lhs.is_constant() and expr.rhs.is_constant():
            lval = expr.lhs.get_constant_value()
            rval = expr.rhs.get_constant_value()
            if expr.op == '+':
                return NumberExprAST(lval + rval, expr.line, expr.col)
            elif expr.op == '-':
                return NumberExprAST(lval - rval, expr.line, expr.col)
            elif expr.op == '*':
                return NumberExprAST(lval * rval, expr.line, expr.col)
            elif expr.op == '/':
                if rval == 0:
                    return expr # Safe: do not fold division by zero
                return NumberExprAST(lval // rval, expr.line, expr.col)
    return expr

def optimize_prog(prog):
    for stmt in prog.statements:
        if isinstance(stmt, VarDeclStmtAST):
            stmt.init_expr = fold_expr(stmt.init_expr)
        elif isinstance(stmt, AssignStmtAST):
            stmt.expr = fold_expr(stmt.expr)
        elif isinstance(stmt, PrintStmtAST):
            stmt.expr = fold_expr(stmt.expr)

# ============================================================================
# IR Generator
# ============================================================================
class IRGenerator:
    def __init__(self):
        self.counter = 0
        self.instructions = []

    def new_temp(self):
        t = f"t{self.counter}"
        self.counter += 1
        return t

    def gen_expr(self, expr):
        if isinstance(expr, NumberExprAST):
            return str(expr.val)
        if isinstance(expr, VariableExprAST):
            return expr.name
        if isinstance(expr, BinaryExprAST):
            left = self.gen_expr(expr.lhs)
            right = self.gen_expr(expr.rhs)
            temp = self.new_temp()
            self.instructions.append(f"{temp} = {left} {expr.op} {right}")
            return temp
        return ""

    def generate(self, prog):
        for stmt in prog.statements:
            if isinstance(stmt, VarDeclStmtAST):
                if stmt.init_expr:
                    val = self.gen_expr(stmt.init_expr)
                    self.instructions.append(f"{stmt.name} = {val}")
            elif isinstance(stmt, AssignStmtAST):
                if stmt.expr:
                    val = self.gen_expr(stmt.expr)
                    self.instructions.append(f"{stmt.name} = {val}")
            elif isinstance(stmt, PrintStmtAST):
                if stmt.expr:
                    val = self.gen_expr(stmt.expr)
                    self.instructions.append(f"PRINT {val}")
        return "\n".join(self.instructions)

# ============================================================================
# Verification Suite Execution
# ============================================================================
def run_test(name, filepath, should_succeed, expected_err_sub):
    with open(filepath, 'r') as f:
        source = f.read()

    lexer = Lexer(source)
    tokens = lexer.tokenize()
    if lexer.errors:
        if not should_succeed:
            for err in lexer.errors:
                if expected_err_sub in err:
                    return True, "Caught expected lexer error"
            return False, f"Lexer error mismatch: {lexer.errors}"
        return False, f"Unexpected lexer error: {lexer.errors[0]}"

    parser = Parser(tokens)
    prog = parser.parse()
    if parser.errors:
        if not should_succeed:
            for err in parser.errors:
                if expected_err_sub in err:
                    return True, "Caught expected parser error"
            return False, f"Parser error mismatch: {parser.errors}"
        return False, f"Unexpected parser error: {parser.errors[0]}"

    semantic = SemanticAnalyzer()
    ok = semantic.analyze(prog)
    if not ok:
        if not should_succeed:
            for err in semantic.errors:
                if expected_err_sub in err:
                    return True, "Caught expected semantic error"
            return False, f"Semantic error mismatch: {semantic.errors}"
        return False, f"Unexpected semantic error: {semantic.errors[0]}"

    if not should_succeed:
        return False, "Expected compilation to fail, but it succeeded!"

    optimize_prog(prog)
    ir_gen = IRGenerator()
    ir_code = ir_gen.generate(prog)

    if "Constant Folding" in name:
        if "folded = 14" not in ir_code:
            return False, f"Failed to fold 2 + 3 * 4 to 14. IR:\n{ir_code}"

    if "Division by Zero" in name:
        if "/ 0" not in ir_code:
            return False, f"Division by zero was unexpectedly modified. IR:\n{ir_code}"

    return True, ir_code

if __name__ == '__main__':
    base_dir = os.path.dirname(__file__)

    # Test basic.mc first
    basic_path = os.path.join(base_dir, "..", "examples", "basic.mc")
    print("Testing examples/basic.mc:")
    ok, output = run_test("Basic Example", basic_path, True, "")
    print(f"Status: {'PASS' if ok else 'FAIL'}")
    print("Generated IR:")
    print(output)
    print("\n" + "="*60 + "\n")

    test_cases = [
        ("01_Valid_Declarations",    os.path.join(base_dir, "01_valid_basic.mc"),          True,  ""),
        ("02_Operator_Precedence",   os.path.join(base_dir, "02_operator_precedence.mc"), True,  ""),
        ("03_Constant_Folding",      os.path.join(base_dir, "03_constant_folding.mc"),    True,  ""),
        ("04_Undeclared_Variable",   os.path.join(base_dir, "04_undeclared_variable.mc"), False, "undeclared"),
        ("05_Duplicate_Declaration", os.path.join(base_dir, "05_duplicate_declaration.mc"), False, "Redefinition"),
        ("06_Missing_Semicolon",     os.path.join(base_dir, "06_missing_semicolon.mc"),   False, "Expected ';'"),
        ("07_Invalid_Character",     os.path.join(base_dir, "07_invalid_char.mc"),        False, "Invalid character"),
        ("08_Division_by_Zero_Safe", os.path.join(base_dir, "08_div_by_zero.mc"),         True,  "")
    ]

    print("Running Full Compiler Test Suite:")
    passed = 0
    for name, path, should_succeed, expected_err in test_cases:
        ok, msg = run_test(name, path, should_succeed, expected_err)
        status = "PASS" if ok else "FAIL"
        if ok: passed += 1
        print(f"[{status}] {name}")
        if not ok:
            print(f"    --> {msg}")

    print("="*60)
    print(f"Total: {passed}/{len(test_cases)} tests passed.")
    if passed == len(test_cases):
        print("ALL TESTS PASSED SUCCESSFULLY!")
    sys.exit(0 if passed == len(test_cases) else 1)
