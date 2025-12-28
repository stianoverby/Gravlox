#include "interpreter.h"

static Allocator global_allocator = {global_malloc, global_calloc, global_free, 0};

Opinfo operator_table[] =
{ {KIND_STAR         , 70, ASSOC_LEFT}
, {KIND_SLASH        , 70, ASSOC_LEFT}
, {KIND_PLUS         , 60, ASSOC_LEFT}
, {KIND_MINUS        , 60, ASSOC_LEFT}
, {KIND_LESS         , 50, ASSOC_LEFT}
, {KIND_LESS_EQUAL   , 50, ASSOC_LEFT}
, {KIND_GREATER_EQUAL, 50, ASSOC_LEFT}
, {KIND_GREATER      , 50, ASSOC_LEFT}
, {KIND_EQUAL_EQUAL  , 40, ASSOC_LEFT}
, {KIND_BANG_EQUAL   , 40, ASSOC_LEFT}
// , {KIND_AND          , 30, ASSOC_LEFT}
// , {KIND_OR           , 20, ASSOC_LEFT}
// , {KIND_EQUAL        , 10, ASSOC_RIGHT}
};

typedef struct mapping Mapping;
struct mapping
{
	String k;
	Token_Kind v;
};

static Mapping keywords[] = {
	{str("and"), KIND_AND},
	{str("class"), KIND_CLASS},
	{str("else"), KIND_ELSE},
	{str("false"), KIND_FALSE},
	{str("for"), KIND_FOR},
	{str("fun"), KIND_FUN},
	{str("if"), KIND_IF},
	{str("nil"), KIND_NIL},
	{str("or"), KIND_OR},
	{str("print"), KIND_PRINT},
	{str("return"), KIND_RETURN},
	{str("super"), KIND_SUPER},
	{str("this"), KIND_THIS},
	{str("true"), KIND_TRUE},
	{str("var"), KIND_VAR},
	{str("while"), KIND_WHILE}};

static bool char_is_digit(char c)
{
	return '0' <= c && c <= '9';
}

static bool char_is_alpha(char c)
{
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

static bool char_is_alpha_numeric(char c)
{
	return char_is_digit(c) || char_is_alpha(c);
}

static bool token_is_unary_op(Token_Kind k)
{
	switch (k)
	{
	case KIND_BANG:
	case KIND_MINUS:
		return true;
	default:
		return false;
	}
}

static bool token_is_binary_op(Token_Kind k)
{
	switch (k)
	{
	case KIND_STAR:
	case KIND_SLASH:
	case KIND_PLUS:
	case KIND_MINUS:
	case KIND_LESS:
	case KIND_LESS_EQUAL:
	case KIND_GREATER_EQUAL:
	case KIND_GREATER:
	case KIND_EQUAL_EQUAL:
	case KIND_BANG_EQUAL:
		return true;
	default:
		return false;
	}
}

static Token_Vector token_vector_new(void)
{
	Token_Vector v;
	v.content = NULL;
	v.capacity = 0;
	v.size = 0;
	return v;
}

static void token_vector_init(Allocator *allocator, Token_Vector *v)
{
	size_t new_capacity;

	new_capacity = sizeof(v->content) * (1UL << 10);

	v->content = allocator->malloc(new_capacity, allocator->ctx);
	panic_if_not(v->content != NULL);

	v->capacity = new_capacity;
	v->size = 0;
}


static void token_vector_append(Allocator *a, Token_Vector *v, Token t)
{
	if(v->capacity < v->size + 1) {
		Token *new = a->malloc(v->capacity * 2, a->ctx);
		panic_if_not(new != NULL);

		memcpy(new, v->content, v->capacity);
		v->capacity = v->capacity * 2;

		a->free(v->content, a->ctx);
		v->content = new;
	}
	v->content[v->size++] = t;
}

static Token* token_vector_get(Token_Vector *v, unsigned int index)
{
	panic_if_not(v->size > index && "index out of bounds");
	return &v->content[index];
}

static Statement_Ptr_Vector statement_vector_new(void)
{
	Statement_Ptr_Vector v;
	v.content = NULL;
	v.capacity = 0;
	v.size = 0;
	return v;
}

static void statement_vector_init(Allocator *allocator, Statement_Ptr_Vector *v)
{
	size_t new_capacity;

	new_capacity = sizeof(v->content) * (1UL << 10);

	v->content = allocator->malloc(new_capacity, allocator->ctx);
	panic_if_not(v->content != NULL);

	v->capacity = new_capacity;
	v->size = 0;
}

static void statement_vector_append(Allocator *a, Statement_Ptr_Vector *v, Statement *s)
{
	if(v->capacity < v->size + 1) {
		Statement **new = a->malloc(v->capacity * 2, a->ctx);
		panic_if_not(new != NULL);

		memcpy(new, v->content, v->capacity);
		v->capacity = v->capacity * 2;

		a->free(v->content, a->ctx);
		v->content = new;
	}
	v->content[v->size++] = s;
}

static Statement* statement_vector_get(Statement_Ptr_Vector *v, unsigned int index)
{
	panic_if_not(v->size > index && "index out of bounds");
	return v->content[index];
}


const char *token_kind_str(Token_Kind kind)
{
	switch (kind)
	{
	case KIND_SLASH:
		return "/";
	case KIND_LEFT_PAREN:
		return "(";
	case KIND_RIGHT_PAREN:
		return ")";
	case KIND_LEFT_BRACE:
		return "{";
	case KIND_RIGHT_BRACE:
		return "}";
	case KIND_COMMA:
		return ",";
	case KIND_DOT:
		return ".";
	case KIND_MINUS:
		return "-";
	case KIND_PLUS:
		return "+";
	case KIND_SEMICOLON:
		return ";";
	case KIND_STAR:
		return "*";
	case KIND_BANG:
		return "!";
	case KIND_EQUAL:
		return "=";
	case KIND_GREATER:
		return ">";
	case KIND_LESS:
		return "<";
	case KIND_BANG_EQUAL:
		return "!=";
	case KIND_EQUAL_EQUAL:
		return "==";
	case KIND_GREATER_EQUAL:
		return ">=";
	case KIND_LESS_EQUAL:
		return "<=";
	case KIND_IDENTIFIER:
		return "<identifier>";
	case KIND_STRING:
		return "<string>";
	case KIND_NUMBER:
		return "<number>";
	case KIND_AND:
		return "and";
	case KIND_CLASS:
		return "class";
	case KIND_ELSE:
		return "else";
	case KIND_FALSE:
		return "false";
	case KIND_TRUE:
		return "true";
	case KIND_FUN:
		return "function";
	case KIND_FOR:
		return "for";
	case KIND_IF:
		return "if";
	case KIND_NIL:
		return "nil";
	case KIND_OR:
		return "or";
	case KIND_PRINT:
		return "print";
	case KIND_RETURN:
		return "return";
	case KIND_SUPER:
		return "super";
	case KIND_THIS:
		return "this";
	case KIND_VAR:
		return "var";
	case KIND_WHILE:
		return "while";
	case KIND_END_OF_FILE:
		return "eof";
	case KIND_UNKNOWN:
		panic_if_not(false && "unknown token kind");
		return "<unknown-token>";
	}
	panic_if_not(false && "unreachable code");
}

void token_print_token(Token *t)
{
	printf("{");
	printf(".kind: %s", token_kind_str(t->kind));
	switch (t->kind)
	{
	case KIND_IDENTIFIER:
	case KIND_STRING:
		printf(" .literal: %.*s", (int)t->literal.as.text.length, t->literal.as.text.content);
		printf(" .literal_size: %ld", t->literal.as.text.length);
		break;
	case KIND_NUMBER:
		printf(" .literal: %f", t->literal.as.number);
		break;
	default:
		// Do nothing
		break;
	}
	printf(" .line: %lu .start: %lu .end: %lu}\n", t->line, t->lexeme_span.start, t->lexeme_span.end);
}

static Scanner scanner_new(String src, Arena *arena)
{
	Scanner scanner;
	scanner.src = src;
	scanner.tokens = token_vector_new();
	token_vector_init(&global_allocator, &scanner.tokens);
	scanner.start_cursor = 0;
	scanner.current_cursor = 0;
	scanner.beginning_of_line = 0;
	scanner.line = 1;
	scanner.had_error = false;
	scanner.arena = arena;
	return scanner;
}

static void scanner_destroy(Scanner *scanner)
{
	global_allocator.free(scanner->tokens.content, global_allocator.ctx);
}

static void scanner_report(Scanner *s, int line, const char *where, const char *message)
{
	fprintf(stderr, "[line %d] Error%s: %s", line, where, message);
	s->had_error = true;
}

static void scanner_error(Scanner *s, int line, const char *message)
{
	scanner_report(s, line, "", message);
}

static bool scanner_is_at_end(Scanner *s)
{
	return s->current_cursor >= (s->src.length);
}

static char scanner_advance(Scanner *s)
{
	return s->src.content[s->current_cursor++];
}

static char scanner_peek(Scanner *s)
{
	if (scanner_is_at_end(s))
	{
		return '\0';
	}
	return s->src.content[s->current_cursor];
}

static char scanner_peek_next(Scanner *s)
{
	if (s->current_cursor + 1 >= s->src.length)
	{
		return '\0';
	}
	return s->src.content[s->current_cursor + 1];
}

static bool scanner_match(Scanner *s, char expected)
{
	if (scanner_is_at_end(s))
	{
		return false;
	}
	if (s->src.content[s->current_cursor] != expected)
	{
		return false;
	}
	s->current_cursor++;
	return true;
}

static void scanner_add_nonvalued_token(Scanner *s, Token_Kind kind)
{
	Token t = {0};
	t.kind = kind;
	t.line = s->line;
	t.lexeme_span.start = s->start_cursor;
	t.lexeme_span.end = s->current_cursor;

	s->start_cursor = s->current_cursor;

	token_vector_append(&global_allocator, &s->tokens, t);
}

static void scanner_add_valued_token(Scanner *s, Token_Kind kind, Value literal)
{
	Token t;
	t.kind = kind;
	t.line = s->line;
	t.literal = literal;
	t.lexeme_span.start = s->start_cursor;
	t.lexeme_span.end = s->current_cursor;

	s->start_cursor = s->current_cursor;

	token_vector_append(&global_allocator, &s->tokens, t);
}

static void scanner_skip_comment(Scanner *s)
{
	while (!scanner_is_at_end(s) && scanner_peek(s) != '\n')
	{
		scanner_advance(s);
	}
}

static void scanner_scan_string(Scanner *s)
{
	while (scanner_peek(s) != '"' && !scanner_is_at_end(s))
	{
		if (scanner_peek(s) == '\n')
		{
			s->line += 1;
		}
		scanner_advance(s);
	}

	if (scanner_is_at_end(s))
	{
		printf("[ERROR]: Unterminated string\n");
		return;
	}
	/* Grab the last " */
	scanner_advance(s);

	Value v = {0};
	v.as.text = string_substring(s->src, s->start_cursor, s->current_cursor);
	panic_if_not(v.as.text.length != 0 && "unable to scan string");

	scanner_add_valued_token(s, KIND_STRING, v);
}

static void scanner_scan_number(Scanner *s)
{
	char c1, c2;
	c1 = scanner_peek(s);
	while (char_is_digit(c1))
	{
		(void)scanner_advance(s);
		c1 = scanner_peek(s);
	}

	c2 = scanner_peek_next(s);
	if (c1 == '.' && char_is_digit(c2))
	{
		(void)scanner_advance(s);
		c1 = scanner_peek(s);
		while (char_is_digit(c1))
		{
			(void)scanner_advance(s);
			c1 = scanner_peek(s);
		}
	}
	Value v = {0};
	v.as.number = strtod(&s->src.content[s->start_cursor], NULL);
	panic_if_not(v.as.number != HUGE_VAL && "unable to convert string to double");
	scanner_add_valued_token(s, KIND_NUMBER, v);
}

static void scanner_scan_identifier(Scanner *s)
{
	char c = scanner_peek(s);
	while (char_is_alpha_numeric(c))
	{
		scanner_advance(s);
		c = scanner_peek(s);
	}

	String text = string_substring(s->src, s->start_cursor, s->current_cursor);
	Token_Kind kind = KIND_UNKNOWN;
	for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
	{
		Mapping kv = keywords[i];
		if (strncmp(text.content, kv.k.content, MIN(text.length, kv.k.length)) == 0)
		{
			kind = kv.v;
			break;
		}
	}

	if (kind == KIND_TRUE || kind == KIND_FALSE) {
		Value v;
		v.kind = VALUE_KIND_BOOLEAN;
		v.as.boolean = (kind == KIND_TRUE) ? true : false;
		scanner_add_valued_token(s, kind, v);
		return;
	}

	if (kind == KIND_UNKNOWN)
	{
		kind = KIND_IDENTIFIER;
	}
	scanner_add_nonvalued_token(s, kind);
}

static void scanner_scan_token(Scanner *s)
{
	Token_Kind kind;
	char c = scanner_advance(s);

	switch (c)
	{
	case '/':
		scanner_match(s, '/')
			? scanner_skip_comment(s)
			: scanner_add_nonvalued_token(s, KIND_SLASH);
		break;
	case ' ':
	case '\r':
	case '\t':
		break;
	case '\n':
		s->line += 1;
		break;
	case '(':
		scanner_add_nonvalued_token(s, KIND_LEFT_PAREN);
		break;
	case ')':
		scanner_add_nonvalued_token(s, KIND_RIGHT_PAREN);
		break;
	case '{':
		scanner_add_nonvalued_token(s, KIND_LEFT_BRACE);
		break;
	case '}':
		scanner_add_nonvalued_token(s, KIND_RIGHT_BRACE);
		break;
	case ',':
		scanner_add_nonvalued_token(s, KIND_COMMA);
		break;
	case '.':
		scanner_add_nonvalued_token(s, KIND_DOT);
		break;
	case '-':
		scanner_add_nonvalued_token(s, KIND_MINUS);
		break;
	case '+':
		scanner_add_nonvalued_token(s, KIND_PLUS);
		break;
	case ';':
		scanner_add_nonvalued_token(s, KIND_SEMICOLON);
		break;
	case '*':
		scanner_add_nonvalued_token(s, KIND_STAR);
		break;
	case '!':
		kind = scanner_match(s, '=') ? KIND_BANG_EQUAL : KIND_BANG;
		scanner_add_nonvalued_token(s, kind);
		break;
	case '=':
		kind = scanner_match(s, '=') ? KIND_EQUAL_EQUAL : KIND_EQUAL;
		scanner_add_nonvalued_token(s, kind);
		break;
	case '>':
		kind = scanner_match(s, '=') ? KIND_GREATER_EQUAL : KIND_GREATER;
		scanner_add_nonvalued_token(s, kind);
		break;
	case '<':
		kind = scanner_match(s, '=') ? KIND_LESS_EQUAL : KIND_LESS;
		scanner_add_nonvalued_token(s, kind);
		break;
	case '"':
		scanner_scan_string(s);
		break;
	default:
		if (char_is_digit(c))
		{
			scanner_scan_number(s);
		}
		else if (char_is_alpha(c))
		{
			scanner_scan_identifier(s);
		}
		else
		{
			panic_if_not(false && "unexpected symbol");
			scanner_error(s, s->line, "unexpected symbol\n");
		}
	}
}

MAYBE_UNUSED static void scanner_print_scanner(Scanner *s)
{
	printf("------------------------------------------\n");
	printf("Scanner:\n");
	printf("  src:");
	string_print_raw(s->src);
	printf("\n");
	printf("  tokens:");
	if (!s->tokens.size)
	{
		printf(" []\n");
	}
	else
	{
		printf(" [\n");
		for(unsigned int i = 0; i < s->tokens.size; i++)
		{
			printf("    ");
			token_print_token(&s->tokens.content[i]);
		}
		printf("  ]");
	}
	printf("\n");
	printf("  start_cursor: %lu\n", s->start_cursor);
	printf("  current_cursor: %lu\n", s->current_cursor);
	printf("  beginning_of_line: %lu\n", s->beginning_of_line);
	printf("  line: %lu\n", s->line);
	printf("------------------------------------------\n");
}

String file_read_to_buffer(Arena *arena, const char *filename)
{
	FILE *fp;
	int rc;
	long file_size;
	long read_size;

	fp = fopen(filename, "rb");
	panic_if_not(fp != NULL && "unable to read file");

	rc = fseek(fp, 0, SEEK_END);
	panic_if_not(rc == 0 && "unable to seek to eof");

	file_size = ftell(fp);
	panic_if_not(0 <= file_size && "unable to tell file size");

	rewind(fp);

	String buffer = string_new(arena, file_size);
	read_size = fread(
		(char *)buffer.content,
		sizeof(char),
		file_size,
		fp);
	panic_if_not(file_size == read_size && "file size should be equal to read size");

	fclose(fp);
	return buffer;
}

void scanner_run_scanner(Scanner *s)
{
	while(!scanner_is_at_end(s)) {
		s->start_cursor = s->current_cursor;
		scanner_scan_token(s);
	}
	// Add eof token
	Token t = {.kind = KIND_END_OF_FILE};
	token_vector_append(&global_allocator, &s->tokens, t);
}

Scanner scanner_scan_file(Arena *arena, const char *filename)
{
	String src = file_read_to_buffer(arena, filename);
	Scanner s = scanner_new(src, arena);
	scanner_run_scanner(&s);
	return s;
}

static size_t operator_lookup_precedence(Token_Kind kind)
{
	for (size_t i = 0; i < sizeof(operator_table); i++)
	{
		if (operator_table[i].kind == kind)
		{
			return operator_table[i].precedence;
		}
	}
	panic_if_not(false && "invalid token precedence look up");
}

static Assoc operator_lookup_assoc(Token_Kind kind)
{
	for (size_t i = 0; i < sizeof(operator_table); i++)
	{
		if (operator_table[i].kind == kind)
		{
			return operator_table[i].assoc;
		}
	}
	panic_if_not(false && "invalid token assoc look up");
}

Parser parser_new(Scanner s)
{
	Parser p = {0};
	p.scanner = s;
	return p;
}

void parser_destroy(Parser *p)
{
	scanner_destroy(&p->scanner);
}

static Token *parser_peek(Parser *p)
{
	return token_vector_get(&p->scanner.tokens, p->pos);
}

static bool parser_is_at_end(Parser *p)
{
	return parser_peek(p)->kind == KIND_END_OF_FILE;
}

static Token *parser_advance(Parser *p)
{
	return parser_is_at_end(p)
			   ? token_vector_get(&p->scanner.tokens, p->pos)
			   : token_vector_get(&p->scanner.tokens, p->pos++);
}

static Token *parser_previous(Parser *p)
{
	return token_vector_get(&p->scanner.tokens, p->pos - 1);
}

static Token *parser_consume(Parser *p, Token_Kind k, String msg)
{
	Token *t = parser_peek(p);
	if (t->kind == k)
	{
		return parser_advance(p);
	}
	fprintf(stderr, "[ERROR] Got '%s', %.*s\n", token_kind_str(t->kind), (int)msg.length, msg.content);
	panic_if_not(false && "unable to consume token");
}

// Parse expressions

// Forward decl because of circular call chain
static Expr *parse_expr(Parser *p);
static Expr *parser_parse_primary_expr(Parser *p)
{
	Token *t = parser_advance(p);
	switch (t->kind)
	{
	case KIND_NUMBER:
	case KIND_STRING:
	case KIND_TRUE:
	case KIND_FALSE:
	case KIND_NIL:
	{
		Expr *expr = arena_push(p->scanner.arena, sizeof(Expr), align_of(Expr));
		Primary_Expr *primary = arena_push(p->scanner.arena, sizeof(Primary_Expr), align_of(Primary_Expr));

		primary->token = t;

		expr->kind = EXPR_PRIMARY;
		expr->as.primary = primary;
		return expr;
	}
	case KIND_LEFT_PAREN:
	{
		Expr *expr = parse_expr(p);
		if (expr == NULL)
		{
			panic_if_not(expr != NULL && "unable to parse expr inside of parens\n");
			return NULL;
		}
		String msg = str("expected ')' after expression.");
		parser_consume(p, KIND_RIGHT_PAREN, msg);
		return expr;
	}
	default:
		panic_if_not(false && "expected primary expression");
		return NULL;
	}
}

static Expr *parser_parse_unary_expr(Parser *p)
{
	Token *t = parser_peek(p);
	if (token_is_unary_op(t->kind))
	{

		Expr *expr;
		Unary_Expr *unary;

		// Skip peeked operator
		(void)parser_advance(p);

		expr = arena_push(p->scanner.arena, sizeof(Expr), align_of(Expr));
		unary = arena_push(p->scanner.arena, sizeof(Unary_Expr), align_of(Unary_Expr));

		unary->operator = t;
		unary->operand = parser_parse_unary_expr(p);
		if (unary->operand == NULL)
		{
			panic_if_not(unary->operand != NULL && "failed to parse operand of unary expr");
			return NULL;
		}
		expr->kind = EXPR_UNARY;
		expr->as.unary = unary;

		return expr;
	}
	return parser_parse_primary_expr(p);
}

static Expr *parser_parse_binary_expr(Parser *p, size_t prev_precedence)
{
	Expr *left = parser_parse_unary_expr(p);
	if (left == NULL)
	{
		panic_if_not(left != NULL && "unable to parse lhs of binary expr");
		return NULL;
	}
	Token *t = parser_peek(p);

	while (token_is_binary_op(t->kind))
	{

		// If the precedence is lower than the previous one, we
		// have to parse the operator higher in the tree
		size_t precedence = operator_lookup_precedence(t->kind);
		if (precedence < prev_precedence)
		{
			break;
		}
		Assoc associativity = operator_lookup_assoc(t->kind);
		size_t new_precedence = precedence + (associativity == ASSOC_LEFT ? 1 : 0);

		// Skip peeked operator
		(void)parser_advance(p);

		Expr *expr = arena_push(p->scanner.arena, sizeof(Expr), align_of(Expr));
		Binary_Expr *binary = arena_push(p->scanner.arena, sizeof(Binary_Expr), align_of(Binary_Expr));

		binary->left = left;
		binary->operator = t;
		binary->right = parser_parse_binary_expr(p, new_precedence);
		if (binary->right == NULL)
		{
			panic_if_not(binary->right != NULL && "unable to parse rhs of binary expr");
			return NULL;
		}
		expr->kind = EXPR_BINARY;
		expr->as.binary = binary;

		left = expr;
		t = parser_peek(p);
	}
	return left;
}

static Expr *parse_expr(Parser *p)
{
	return parser_parse_binary_expr(p, 0);
}

static Expr *parse_stmt_print(Parser *p)
{
	panic_if_not(parser_advance(p)->kind == KIND_PRINT);
	Expr *expr = parse_expr(p);
	return expr;
}

static Expr *parse_stmt_expr(Parser *p)
{
	return parse_expr(p);
}

static Statement *parse_stmt(Parser *p)
{
	Statement *stmt = arena_push(p->scanner.arena, sizeof(Statement), align_of(Statement));
	panic_if_not(stmt != NULL);

	switch(parser_peek(p)->kind) {
		case KIND_PRINT:
			stmt->kind = STMT_PRINT;
			stmt->as.expression = parse_stmt_print(p);
			break;
		default:
			stmt->kind = STMT_EXPR;
			stmt->as.expression = parse_stmt_expr(p);
	}
	return stmt;
}

static Statement_Ptr_Vector parse_stmts(Parser *p)
{
	Statement_Ptr_Vector v = statement_vector_new();
	statement_vector_init(&global_allocator, &v);
	while(!parser_is_at_end(p)) {
		Statement *stmt = parse_stmt(p);
		statement_vector_append(&global_allocator, &v, stmt);
		panic_if_not(parser_advance(p)->kind == KIND_SEMICOLON);
	}
	return v;
}

MAYBE_UNUSED static void parser_synchronize(Parser *p)
{
	(void)parser_advance(p);
	while (!parser_is_at_end(p))
	{
		if (parser_previous(p)->kind == KIND_SEMICOLON)
		{
			return;
		}
		switch (parser_peek(p)->kind)
		{
		case KIND_CLASS:
		case KIND_FOR:
		case KIND_FUN:
		case KIND_IF:
		case KIND_PRINT:
		case KIND_RETURN:
		case KIND_VAR:
		case KIND_WHILE:
			return;
		default:
			(void)parser_advance(p);
		}
	}
}

// Pretty printing (pp)

// Forward decl because of circular call chain
void expr_print(Expr *e, FILE *out);

void expr_print_primary(Primary_Expr *e, FILE *out)
{
	switch (e->token->kind)
	{
	case KIND_NUMBER:
		fprintf(out, "%f", e->token->literal.as.number);
		break;
	case KIND_STRING:
		fprintf(out, "%.*s", (int)e->token->literal.as.text.length, e->token->literal.as.text.content);
		break;
	case KIND_TRUE:
		fprintf(out, "true");
		break;
	case KIND_FALSE:
		fprintf(out, "false");
		break;
	case KIND_NIL:
		fprintf(out, "nil");
		break;
	default:
		panic_if_not(false && "case not yet implemented");
	}
}
void expr_print_unary(Unary_Expr *e, FILE *out)
{
	fprintf(out, "(");
	switch (e->operator->kind)
	{
	case KIND_MINUS:
		fprintf(out, "-");
		break;
	case KIND_BANG:
		fprintf(out, "!");
		break;
	default:
		panic_if_not(false && "case not yet implemented");
	}
	expr_print(e->operand, out);
	fprintf(out, ")");
}

void expr_print_binary(Binary_Expr *e, FILE *out)
{
	fprintf(out, "(");
	expr_print(e->left, out);
	switch (e->operator->kind)
	{
	case KIND_STAR:
		fprintf(out, " * ");
		break;
	case KIND_SLASH:
		fprintf(out, " / ");
		break;
	case KIND_PLUS:
		fprintf(out, " + ");
		break;
	case KIND_MINUS:
		fprintf(out, " - ");
		break;
	case KIND_LESS:
		fprintf(out, " < ");
		break;
	case KIND_LESS_EQUAL:
		fprintf(out, " <= ");
		break;
	case KIND_GREATER_EQUAL:
		fprintf(out, " >= ");
		break;
	case KIND_GREATER:
		fprintf(out, " > ");
		break;
	case KIND_BANG_EQUAL:
		fprintf(out, " != ");
		break;
	case KIND_EQUAL_EQUAL:
		fprintf(out, " == ");
		break;
	default:
		panic_if_not(false && "case not yet implemented");
	}
	expr_print(e->right, out);
	fprintf(out, ")");
}

void expr_print(Expr *e, FILE *out)
{
	switch (e->kind)
	{
	case EXPR_PRIMARY:
		expr_print_primary(e->as.primary, out);
		break;
	case EXPR_UNARY:
		expr_print_unary(e->as.unary, out);
		break;
	case EXPR_BINARY:
		expr_print_binary(e->as.binary, out);
		break;
	default:
		panic_if_not(false && "expr has no type");
	}
}

void stmt_print(Statement *stmt, FILE *out)
{
	switch (stmt->kind)
	{
	case STMT_PRINT:
		fprintf(out, "print(");
		expr_print(stmt->as.expression, out);
		fprintf(out, ")");
		break;
	case STMT_EXPR:
		expr_print(stmt->as.expression, out);
		break;
	default:
		panic_if_not(false && "stmt has no type");
	}
}

void value_print(Value *v, FILE *out)
{
	switch (v->kind)
	{
	case VALUE_KIND_NULL:
		fprintf(out, "NULL");
		break;
	case VALUE_KIND_STRING:
		fprintf(out, "%.*s", (int)v->as.text.length, v->as.text.content);
		break;
	case VALUE_KIND_NUMBER:
		fprintf(out, "%lf", v->as.number);
		break;
	case VALUE_KIND_BOOLEAN:
		fprintf(out, "%s", (v->as.boolean) ? "true" : "false");
		break;
	default:
		panic_if_not(false && "value has no type");
	}
}

Value interpreter_eval_expr(Arena *arena, Expr *expr);

Value interpreter_eval_primary(Primary_Expr *primary)
{
	Value v = {0};
	switch (primary->token->kind)
	{
	case KIND_NUMBER:
		v.kind = VALUE_KIND_NUMBER;
		v.as.number = primary->token->literal.as.number;
		break;
	case KIND_STRING:
		v.kind = VALUE_KIND_STRING;
		v.as.text = primary->token->literal.as.text;
		break;
	case KIND_TRUE:
	case KIND_FALSE:
		v.kind = VALUE_KIND_BOOLEAN;
		v.as.boolean = primary->token->literal.as.boolean;
		break;
	case KIND_NIL:
		v.kind = VALUE_KIND_NULL;
		break;
	default:
		panic_if_not(false && "invalid primary expr");
	}
	return v;
}
Value interpreter_eval_unary(Arena *arena, Unary_Expr *unary)
{
	Value v = {0};
	switch (unary->operator->kind)
	{
	case KIND_MINUS:
		v = interpreter_eval_expr(arena, unary->operand);
		if (v.kind != VALUE_KIND_NUMBER)
		{
			// TODO: error handling
			panic_if_not(v.kind == VALUE_KIND_NUMBER && "expected number");
		}
		break;
	case KIND_BANG:
		v = interpreter_eval_expr(arena, unary->operand);
		if (v.kind != VALUE_KIND_NUMBER || v.kind != VALUE_KIND_BOOLEAN)
		{
			// TODO: error handling
			panic_if_not(v.kind == VALUE_KIND_NUMBER && "expected number or boolean");
		}
		break;
	default:
		panic_if_not(false && "invalid unary expr");
		break;
	}
	return v;
}
Value interpreter_eval_binary(Arena *arena, Binary_Expr *expr)
{
	Value result = {0};
	Value left = interpreter_eval_expr(arena, expr->left);
	Value right = interpreter_eval_expr(arena, expr->right);
	switch (expr->operator->kind)
	{
	case KIND_STAR:
		if (left.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected lhs of '*' to be number");
		}
		if (right.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected rhs of '*' to be number");
		}
		result.kind = VALUE_KIND_NUMBER;
		result.as.number = left.as.number * right.as.number;
		break;
	case KIND_SLASH:
		if (left.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected lhs of '/' to be number");
		}
		if (right.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected rhs of '/' to be number");
		}
		if (right.as.number == 0)
		{
			panic_if_not(right.as.number != VALUE_KIND_NUMBER && "divide by zero");
		}
		result.kind = VALUE_KIND_NUMBER;
		result.as.number = left.as.number / right.as.number;
		break;
	case KIND_PLUS:
		if (left.kind != right.kind)
		{
			panic_if_not(left.kind == right.kind && "expected lhs and rhs of '+' to be number");
		}
		if (left.kind == VALUE_KIND_NUMBER)
		{
			result.kind = VALUE_KIND_NUMBER;
			result.as.number = left.as.number + right.as.number;
		}
		else if (left.kind == VALUE_KIND_STRING)
		{
			result.kind = VALUE_KIND_STRING;
			result.as.text = string_concat(arena, left.as.text, right.as.text);
		}
		else
		{
			panic_if_not(false && "expected lhs and rhs of '+' to be numbers or strings");
		}
		break;
	case KIND_MINUS:
		if (left.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected lhs of '-' to be number");
		}
		if (right.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected rhs of '-' to be number");
		}
		result.kind = VALUE_KIND_NUMBER;
		result.as.number = left.as.number - right.as.number;
		break;
	case KIND_LESS:
		if (left.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected lhs of '<' to be number");
		}
		if (right.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected rhs of '<' to be number");
		}
		result.kind = VALUE_KIND_BOOLEAN;
		result.as.boolean = left.as.number < right.as.number;
		break;
	case KIND_LESS_EQUAL:
		if (left.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected lhs of '<=' to be number");
		}
		if (right.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected rhs of '<=' to be number");
		}
		result.kind = VALUE_KIND_BOOLEAN;
		result.as.boolean = left.as.number <= right.as.number;
		break;
	case KIND_GREATER_EQUAL:
		if (left.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected lhs of '>=' to be number");
		}
		if (right.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected rhs of '>=' to be number");
		}
		result.kind = VALUE_KIND_BOOLEAN;
		result.as.boolean = left.as.number >= right.as.number;
		break;
	case KIND_GREATER:
		if (left.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected lhs of '>' to be number");
		}
		if (right.kind != VALUE_KIND_NUMBER)
		{
			panic_if_not(left.kind == VALUE_KIND_NUMBER && "expected rhs of '>' to be number");
		}
		result.kind = VALUE_KIND_BOOLEAN;
		result.as.boolean = left.as.number > right.as.number;
		break;
	case KIND_BANG_EQUAL:
		if (left.kind != right.kind || left.kind == VALUE_KIND_NULL || right.kind == VALUE_KIND_NULL)
		{
			panic_if_not(left.kind == right.kind && "expected lhs and rhs of '!=' to be same type");
		}
		if (left.kind == VALUE_KIND_NULL || right.kind == VALUE_KIND_NULL)
		{
			result.kind = VALUE_KIND_BOOLEAN;
			result.as.boolean = left.kind != right.kind;
		}
		else if (left.kind == VALUE_KIND_NUMBER)
		{
			result.kind = VALUE_KIND_BOOLEAN;
			result.as.boolean = left.as.number != right.as.number;
		}
		else if (left.kind == VALUE_KIND_BOOLEAN)
		{
			result.kind = VALUE_KIND_BOOLEAN;
			result.as.boolean = left.as.boolean != right.as.boolean;
		}
		else if (left.kind == VALUE_KIND_STRING)
		{
			result.kind = VALUE_KIND_BOOLEAN;
			result.as.boolean = !string_equals(left.as.text, right.as.text);
		}
		else
		{
			panic_if_not(false && "expected lhs and rhs of '!=' to be numbers or strings");
		}
		break;
	case KIND_EQUAL_EQUAL:
		if (left.kind != right.kind || left.kind == VALUE_KIND_NULL || right.kind == VALUE_KIND_NULL)
		{
			panic_if_not(left.kind == right.kind && "expected lhs and rhs of '==' to be same type");
		}
		if (left.kind == VALUE_KIND_NULL || right.kind == VALUE_KIND_NULL)
		{
			result.kind = VALUE_KIND_BOOLEAN;
			result.as.boolean = left.kind == right.kind;
		}
		else if (left.kind == VALUE_KIND_NUMBER)
		{
			result.kind = VALUE_KIND_BOOLEAN;
			result.as.boolean = left.as.number == right.as.number;
		}
		else if (left.kind == VALUE_KIND_BOOLEAN)
		{
			result.kind = VALUE_KIND_BOOLEAN;
			result.as.boolean = left.as.boolean == right.as.boolean;
		}
		else if (left.kind == VALUE_KIND_STRING)
		{
			result.kind = VALUE_KIND_BOOLEAN;
			result.as.boolean = string_equals(left.as.text, right.as.text);
		}
		else
		{
			panic_if_not(false && "expected lhs and rhs of '==' to be numbers or strings");
		}
		break;
	default:
		panic_if_not(false && "invalid binary expr");
	}
	return result;
}

Value interpreter_eval_expr(Arena *arena, Expr *expr)
{
	switch (expr->kind)
	{
	case EXPR_PRIMARY:
		return interpreter_eval_primary(expr->as.primary);
	case EXPR_UNARY:
		return interpreter_eval_unary(arena, expr->as.unary);
	case EXPR_BINARY:
		return interpreter_eval_binary(arena, expr->as.binary);
	default:
		panic_if_not(false && "expr has no type");
	}
}

void interpreter_builtin_print(Value *v)
{
	switch(v->kind) {
		case VALUE_KIND_BOOLEAN:
		case VALUE_KIND_NULL:
		case VALUE_KIND_NUMBER:
			value_print(v, stdout);
			break;
		case VALUE_KIND_STRING:
			if(v->as.text.length <= 2) {
				fprintf(stdout, "\n");
			} else {
				fprintf(stdout, "%.*s\n", (int)v->as.text.length - 1, &v->as.text.content[1]);
			}
			break;
		default:
			panic_if_not(false && "value has no type");
	}
}

Value interpreter_eval_stmt(Arena *arena, Statement *stmt)
{
	switch (stmt->kind)
	{
	case STMT_PRINT: {
		Value v = interpreter_eval_expr(arena, stmt->as.expression);
		value_print(&v, stdout);
		fprintf(stdout, "\n");
		v.kind = VALUE_KIND_NULL;
		return v;
	}
	case STMT_EXPR: {
		return interpreter_eval_expr(arena, stmt->as.expression);
	}
	default:
		panic_if_not(false && "expr has no type");
	}
}

void interpreter_interpret_file(Arena *arena, const char *filename)
{
	Parser *p;
	Value v;

	p = arena_push(arena, sizeof(Parser), align_of(Parser));
	p->scanner = scanner_scan_file(arena, filename);
	p->pos = 0;

	Statement_Ptr_Vector stms = parse_stmts(p);
	for(unsigned int i = 0; i < stms.size; i++) {
		Statement *stmt = statement_vector_get(&stms, i);
		stmt_print(stmt, stdout);
		printf("\n");
		v = interpreter_eval_stmt(arena, stmt);
		if(v.kind != VALUE_KIND_NULL) {
			value_print(&v, stdout);
			printf("\n");
		}
	}
	global_allocator.free(stms.content, global_allocator.ctx);
	parser_destroy(p);
}