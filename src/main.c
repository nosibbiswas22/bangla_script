#include <stdio.h>

/*
 * BanglaScript single-translation-unit entry point.
 *
 * The full interpreter is included here so the project builds from one
 * compiled source file. The implementation remains unchanged, but it is now
 * compiled through this file only.
 */
#define main bangla_interpreter_main
#include "interpreter.c"
#undef main

int main(int argc, char **argv) {
    return bangla_interpreter_main(argc, argv);
}
