#include "lutils.h"
#include "interpreter.c"

bool test_scanner_run_scanner(void)
{
	char cstr[] =
	    "\t\n\r            \n"  // White space should be ignored
		"/                 \n"  // KIND_SLASH
		"(                 \n"  // KIND_LEFT_PAREN
		")                 \n"  // KIND_RIGHT_PAREN
		"{                 \n"  // KIND_LEFT_BRACE
		"}                 \n"  // KIND_RIGHT_BRACE
		",                 \n"  // KIND_COMMA
		".                 \n"  // KIND_DOT
		"-                 \n"  // KIND_MINUS
		"+                 \n"  // KIND_PLUS
		";                 \n"  // KIND_SEMICOLON
		"*                 \n"  // KIND_STAR
		"!                 \n"  // KIND_BANG
		"=                 \n"  // KIND_EQUAL
		">                 \n"  // KIND_GREATER
		"<                 \n"  // KIND_LESS
		"!=                \n"  // KIND_BANG_EQUAL
		"==                \n"  // KIND_EQUAL_EQUAL
		">=                \n"  // KIND_GREATER_EQUAL
		"<=                \n"  // KIND_LESS_EQUAL
		"identifier        \n"  // KIND_IDENTIFIER
		"\"string literal\"\n"  // KIND_STRING
		"12345             \n"  // KIND_NUMBER
		"and               \n"  // KIND_AND
		"class             \n"  // KIND_CLASS
		"else              \n"  // KIND_ELSE
		"false             \n"  // KIND_FALSE
		"true              \n"  // KIND_TRUE
		"fun               \n"  // KIND_FUN
		"for               \n"  // KIND_FOR
		"if                \n"  // KIND_IF
		"nil               \n"  // KIND_NIL
		"or                \n"  // KIND_OR
		"print             \n"  // KIND_PRINT
		"return            \n"  // KIND_RETURN
		"super             \n"  // KIND_SUPER
		"this              \n"  // KIND_THIS
		"var               \n"  // KIND_VAR
		"while             \n"; // KIND_WHILE

	Arena arena = arena_create(ARENA_DEFAULT_CAPACITY);
	String src = string_cstr(&arena, cstr);
	Scanner s = scanner_new(src, &arena);

	scanner_run_scanner(&s);

	panic_if_not(token_vector_get(&s.tokens,  0)->kind == KIND_SLASH);
	panic_if_not(token_vector_get(&s.tokens,  1)->kind == KIND_LEFT_PAREN);
	panic_if_not(token_vector_get(&s.tokens,  2)->kind == KIND_RIGHT_PAREN);
	panic_if_not(token_vector_get(&s.tokens,  3)->kind == KIND_LEFT_BRACE);
	panic_if_not(token_vector_get(&s.tokens,  4)->kind == KIND_RIGHT_BRACE);
	panic_if_not(token_vector_get(&s.tokens,  5)->kind == KIND_COMMA);
	panic_if_not(token_vector_get(&s.tokens,  6)->kind == KIND_DOT);
	panic_if_not(token_vector_get(&s.tokens,  7)->kind == KIND_MINUS);
	panic_if_not(token_vector_get(&s.tokens,  8)->kind == KIND_PLUS);
	panic_if_not(token_vector_get(&s.tokens,  9)->kind == KIND_SEMICOLON);
	panic_if_not(token_vector_get(&s.tokens, 10)->kind == KIND_STAR);
	panic_if_not(token_vector_get(&s.tokens, 11)->kind == KIND_BANG);
	panic_if_not(token_vector_get(&s.tokens, 12)->kind == KIND_EQUAL);
	panic_if_not(token_vector_get(&s.tokens, 13)->kind == KIND_GREATER);
	panic_if_not(token_vector_get(&s.tokens, 14)->kind == KIND_LESS);
	panic_if_not(token_vector_get(&s.tokens, 15)->kind == KIND_BANG_EQUAL);
	panic_if_not(token_vector_get(&s.tokens, 16)->kind == KIND_EQUAL_EQUAL);
	panic_if_not(token_vector_get(&s.tokens, 17)->kind == KIND_GREATER_EQUAL);
	panic_if_not(token_vector_get(&s.tokens, 18)->kind == KIND_LESS_EQUAL);
	panic_if_not(token_vector_get(&s.tokens, 19)->kind == KIND_IDENTIFIER);
	panic_if_not(token_vector_get(&s.tokens, 20)->kind == KIND_STRING);
	panic_if_not(token_vector_get(&s.tokens, 21)->kind == KIND_NUMBER);
	panic_if_not(token_vector_get(&s.tokens, 22)->kind == KIND_AND);
	panic_if_not(token_vector_get(&s.tokens, 23)->kind == KIND_CLASS);
	panic_if_not(token_vector_get(&s.tokens, 24)->kind == KIND_ELSE);
	panic_if_not(token_vector_get(&s.tokens, 25)->kind == KIND_FALSE);
	panic_if_not(token_vector_get(&s.tokens, 26)->kind == KIND_TRUE);
	panic_if_not(token_vector_get(&s.tokens, 27)->kind == KIND_FUN);
	panic_if_not(token_vector_get(&s.tokens, 28)->kind == KIND_FOR);
	panic_if_not(token_vector_get(&s.tokens, 29)->kind == KIND_IF);
	panic_if_not(token_vector_get(&s.tokens, 30)->kind == KIND_NIL);
	panic_if_not(token_vector_get(&s.tokens, 31)->kind == KIND_OR);
	panic_if_not(token_vector_get(&s.tokens, 32)->kind == KIND_PRINT);
	panic_if_not(token_vector_get(&s.tokens, 33)->kind == KIND_RETURN);
	panic_if_not(token_vector_get(&s.tokens, 34)->kind == KIND_SUPER);
	panic_if_not(token_vector_get(&s.tokens, 35)->kind == KIND_THIS);
	panic_if_not(token_vector_get(&s.tokens, 36)->kind == KIND_VAR);
	panic_if_not(token_vector_get(&s.tokens, 37)->kind == KIND_WHILE);
	panic_if_not(token_vector_get(&s.tokens, 38)->kind == KIND_END_OF_FILE);

	scanner_destroy(&s);
	arena_release(&arena);
	return true;
}

bool test_interpreter_interpret_file()
{
	Arena arena = arena_create(ARENA_DEFAULT_CAPACITY);
	interpreter_interpret_file(&arena, "program.lox");
	arena_release(&arena);
	return true;
}

int main(void)
{
	(void)test_scanner_run_scanner();
	(void)test_interpreter_interpret_file();
	return 0;
}
