#include <stdio.h>
#include <string.h>

#include "interpreter.h"

#ifdef _WIN32
#define TokenType WindowsTokenType
#include <windows.h>
#undef TokenType
#endif

static void repl(void) {
    char line[4096];
    puts("BanglaScript 0.1 - Ctrl+D to exit");
    while (1) {
        fputs(">>> ", stdout);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        bangla_run_source(line);
    }
}

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    if (argc > 2) {
        fprintf(stderr, "usage: %s [file.bs]\n", argv[0]);
        return 2;
    }
    if (argc == 2) return bangla_run_file(argv[1]);
    repl();
    return 0;
}
