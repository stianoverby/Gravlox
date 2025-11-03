# Gravlox - the Lox Programming Language Buried Deep in C.

An experimental C implementation of the [The Lox Programming Language](https://craftinginterpreters.com/the-lox-language.html), built primarily as a playground for exploring C design patterns and dynamically typed programming languages.

> Warning: This project is still under heavy development. Expect missing features, poor error handling, and a general sense of chaos.

---

## Motivation

This project is not meant to be a production-ready interpreter. It is rather an educational exercise in experimenting with:
- Memory management techniques
- Compiler and runtime design patterns

---

## Build Instructions

This implementation currently supports Linux only. I have at least not made an effort to make it run on any other operating system.

### Requirements
- GCC (tested on `gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0`)
- `make`

### Build

```bash
make
```

### Clean

```bash
make clean
```

### Build with Sanitizers and Static Analysis

```bash
make USE_EXTRA_CFLAGS=1
```

Note: The `USE_EXTRA_CFLAGS` option may not be supported by all GCC versions.

---


## Status

- [x] Lexer
- [ ] Parser
  - [x] Expressions
  - [ ] Statements
- [ ] Interpreter
  - [x] Expressions
  - [ ] Statements
- [ ] Proper error handling
- [ ] Virtual machine backend
- [ ] Standard library

---


## License

This project is distributed under the [MIT](https://choosealicense.com/licenses/mit/) license.


---

## Author

Created by Stian Øverby.
Just another curious developer exploring the dark corners of C.