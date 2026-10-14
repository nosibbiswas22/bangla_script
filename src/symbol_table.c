#include "symbol_table.h"

struct BsSymbolTable { BsSymbolTable *parent; };

BsSymbolTable *bangla_symbols_create(BsSymbolTable *parent) {
    (void)parent;
    return 0;
}

void bangla_symbols_destroy(BsSymbolTable *symbols) {
    (void)symbols;
}
