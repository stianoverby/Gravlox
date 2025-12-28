#ifndef _PARSER_H_
#define _PARSER_H_

#include "lutils.h"

enum token_kind
{
	KIND_UNKNOWN,
	KIND_SLASH,
	KIND_LEFT_PAREN,
	KIND_RIGHT_PAREN,
	KIND_LEFT_BRACE,
	KIND_RIGHT_BRACE,
	KIND_COMMA,
	KIND_DOT,
	KIND_MINUS,
	KIND_PLUS,
	KIND_SEMICOLON,
	KIND_STAR,
	KIND_BANG,
	KIND_EQUAL,
	KIND_GREATER,
	KIND_LESS,
	KIND_BANG_EQUAL,
	KIND_EQUAL_EQUAL,
	KIND_GREATER_EQUAL,
	KIND_LESS_EQUAL,
	KIND_IDENTIFIER,
	KIND_STRING,
	KIND_NUMBER,
	KIND_AND,
	KIND_CLASS,
	KIND_ELSE,
	KIND_FALSE,
	KIND_TRUE,
	KIND_FUN,
	KIND_FOR,
	KIND_IF,
	KIND_NIL,
	KIND_OR,
	KIND_PRINT,
	KIND_RETURN,
	KIND_SUPER,
	KIND_THIS,
	KIND_VAR,
	KIND_WHILE,
	KIND_END_OF_FILE
};
typedef enum token_kind Token_Kind;

enum value_kind
{ VALUE_KIND_UNKNOWN
, VALUE_KIND_NULL
, VALUE_KIND_STRING
, VALUE_KIND_NUMBER
, VALUE_KIND_BOOLEAN
};
typedef enum value_kind Value_Kind;

typedef struct value Value;
struct value {
	Value_Kind kind;
	union {
		String text;
		double number;
		bool boolean;
	} as;
};

typedef struct span Span;
struct span
{
	size_t start;
	size_t end;
};

typedef struct token Token;
struct token
{
	Token_Kind kind;
	Span lexeme_span;
	Value literal;
	size_t line;
};

typedef struct token_vector Token_Vector;
struct token_vector
{
	Token *content;
	size_t capacity;
	size_t size;
};

typedef struct scanner Scanner;
struct scanner
{
	String src;
	Token_Vector tokens;
	size_t start_cursor;
	size_t current_cursor;
	size_t beginning_of_line;
	size_t line;
	bool had_error;
	Arena *arena;
};

void scanner_run_scanner(Scanner *s);
Scanner scanner_scan_file(Arena* arena, const char* filename);

void token_print_token(Token *t);
const char *token_kind_str(Token_Kind kind);

enum expr_kind
{ EXPR_PRIMARY
, EXPR_UNARY
, EXPR_BINARY
};
typedef enum expr_kind Expr_Kind;

typedef struct primary_expr Primary_Expr;
typedef struct unary_expr   Unary_Expr;
typedef struct binary_expr  Binary_Expr;
typedef struct expr         Expr;

struct primary_expr {
    Token *token;
};

struct unary_expr {
    Expr  *operand;
    Token *operator;
};

struct binary_expr {
    Expr  *left;
    Expr  *right;
    Token *operator;
};

struct expr {
    Expr_Kind kind;
    union {
        Primary_Expr *primary;
        Unary_Expr   *unary;
        Binary_Expr  *binary;
    } as;
};

enum stmt_kind
{ STMT_EXPR
, STMT_PRINT
};

typedef enum stmt_kind Stmt_Kind;

typedef struct statement Statement;
struct statement {
	Stmt_Kind kind;
	union {
		Expr *expression;
	} as;
};

enum decl_kind
{ DECL_VAR
, DECL_STMT
};
typedef enum decl_kind Decl_Kind;

typedef struct declaration Declaration;
struct declaration {
	Decl_Kind kind;
	union {
		Statement statement;
	} as;
};

typedef struct stmtptr_vector Statement_Ptr_Vector;
struct stmtptr_vector
{
	Statement **content;
	size_t capacity;
	size_t size;
};


typedef struct parser Parser;
struct parser {
	Scanner scanner;
	size_t pos;
};

enum assoc {
    ASSOC_LEFT,
    ASSOC_RIGHT
};
typedef enum assoc Assoc;

typedef struct opinfo Opinfo;
struct opinfo
{
	Token_Kind kind;
	size_t     precedence;
    Assoc      assoc;
};

void interpreter_interpret_file(Arena *arena, const char* filename);

#endif //_PARSER_H_