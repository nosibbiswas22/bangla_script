#ifndef BANGLASCRIPT_PARSER_H
#define BANGLASCRIPT_PARSER_H

typedef struct BsAst BsAst;

/*
 * AST nodes are intentionally opaque outside the parser/interpreter
 * pipeline. This keeps ownership and recursive node details private.
 */
BsAst *bangla_parse(const char *source);
void bangla_ast_free(BsAst *ast);

#endif
