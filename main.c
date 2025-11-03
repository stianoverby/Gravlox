#include "lutils.h"
#include "interpreter.h"

int main(void)
{
	Arena arena = arena_create(ARENA_DEFAULT_CAPACITY);

	interpreter_interpret_file(&arena, "program.lox");

	arena_release(&arena);
	return 0;
}
