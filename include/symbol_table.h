#ifndef BANGLASCRIPT_SYMBOL_TABLE_H
#define BANGLASCRIPT_SYMBOL_TABLE_H

typedef struct BsSymbolTable BsSymbolTable;

BsSymbolTable *bangla_symbols_create(BsSymbolTable *parent);
void bangla_symbols_destroy(BsSymbolTable *symbols);

#endif
