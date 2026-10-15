/*
 * BanglaScript - a small tree-walk compiler/runtime.
 *
 * The front end tokenises UTF-8 source, parses it into an AST, performs
 * execution through a dynamically scoped environment, and reports source
 * locations for syntax and runtime errors.  Bengali identifiers are kept as
 * UTF-8 byte strings; this is sufficient because identifiers are compared
 * byte-for-byte and the language does not split individual Unicode codepoints.
 */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define TokenType WindowsTokenType
#include <windows.h>
#undef TokenType
#endif

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Env Env;
typedef struct Function Function;
typedef struct Class Class;
typedef struct Instance Instance;

typedef enum
{
    T_EOF,
    T_ID,
    T_NUMBER,
    T_STRING,
    T_LEFT_PAREN,
    T_RIGHT_PAREN,
    T_LEFT_BRACE,
    T_RIGHT_BRACE,
    T_COMMA,
    T_SEMICOLON,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_PERCENT,
    T_BANG,
    T_BANG_EQUAL,
    T_EQUAL,
    T_EQUAL_EQUAL,
    T_LESS,
    T_LESS_EQUAL,
    T_GREATER,
    T_GREATER_EQUAL,
    T_AND,
    T_OR,
    T_IF,
    T_ELSE,
    T_WHILE,
    T_FOR,
    T_FUNCTION,
    T_RETURN,
    T_LET,
    T_PRINT,
    T_TRUE,
    T_FALSE,
    T_NULL,
    T_CLASS,
    T_THIS,
    T_SUPER,
    T_DOT
} TokenType;

typedef struct
{
    TokenType type;
    char *text;
    double number;
    int line;
} Token;
typedef struct
{
    const char *source;
    size_t start, current;
    int line;
} Lexer;
typedef struct
{
    Lexer lexer;
    Token current, previous;
    int had_error;
} Parser;

typedef enum
{
    V_NULL,
    V_NUMBER,
    V_BOOL,
    V_STRING,
    V_FUNCTION,
    V_CLASS,
    V_INSTANCE
} ValueType;
typedef struct
{
    ValueType type;
    union
    {
        double number;
        int boolean;
        char *string;
        Function *function;
        Class *class_obj;
        Instance *instance;
    };
} Value;

typedef enum
{
    E_LITERAL,
    E_VARIABLE,
    E_ASSIGN,
    E_UNARY,
    E_BINARY,
    E_LOGICAL,
    E_CALL,
    E_GET,
    E_SET,
    E_THIS,
    E_SUPER
} ExprType;
struct Expr
{
    ExprType type;
    int line;
    union
    {
        Value literal;
        char *name;
        struct
        {
            char *name;
            Expr *value;
        } assign;
        struct
        {
            TokenType op;
            Expr *right;
        } unary;
        struct
        {
            TokenType op;
            Expr *left, *right;
        } binary;
        struct
        {
            TokenType op;
            Expr *left, *right;
        } logical;
        struct
        {
            Expr *callee;
            Expr **args;
            int count;
        } call;
        struct
        {
            Expr *object;
            char *name;
        } get;
        struct
        {
            Expr *object;
            char *name;
            Expr *value;
        } set;
        struct
        {
            char *method;
        } super;
    };
};

typedef enum
{
    S_EXPR,
    S_PRINT,
    S_VAR,
    S_BLOCK,
    S_IF,
    S_WHILE,
    S_FUNCTION,
    S_RETURN,
    S_CLASS
} StmtType;
struct Stmt
{
    StmtType type;
    int line;
    union
    {
        Expr *expression;
        struct
        {
            Expr **items;
            int count;
        } print;
        struct
        {
            char *name;
            Expr *initializer;
        } var;
        struct
        {
            Stmt **items;
            int count;
        } block;
        struct
        {
            Expr *condition;
            Stmt *then_branch, *else_branch;
        } if_stmt;
        struct
        {
            Expr *condition;
            Stmt *body;
        } while_stmt;
        struct
        {
            char *name;
            char **params;
            int param_count;
            Stmt *body;
        } function;
        struct
        {
            Expr *value;
        } return_stmt;
        struct
        {
            char *name;
            char *superclass;
            Stmt **methods;
            int method_count;
        } class_stmt;
    };
};

typedef struct
{
    char *name;
    Value value;
} Binding;
struct Env
{
    Env *parent;
    Binding *items;
    int count, capacity;
};
struct Function
{
    char *name;
    char **params;
    int param_count;
    Stmt *body;
    Env *closure;
    int is_initializer;
};
struct Class
{
    char *name;
    Class *superclass;
    Function **methods;
    int method_count;
};
struct Instance
{
    Class *class_obj;
    Binding *fields;
    int count, capacity;
};

static int error_count;
static void fail_at(int line, const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "BanglaScript:%d: error: ", line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    error_count++;
}
static void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p)
    {
        fputs("BanglaScript: out of memory\n", stderr);
        exit(1);
    }
    return p;
}
static void *xrealloc(void *p, size_t n)
{
    p = realloc(p, n ? n : 1);
    if (!p)
    {
        fputs("BanglaScript: out of memory\n", stderr);
        exit(1);
    }
    return p;
}
static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = xmalloc(n);
    memcpy(p, s, n);
    return p;
}
static char *slice(const char *s, size_t a, size_t b)
{
    char *p = xmalloc(b - a + 1);
    memcpy(p, s + a, b - a);
    p[b - a] = '\0';
    return p;
}

static int is_id_start(unsigned char c) { return c >= 128 || isalpha(c) || c == '_'; }
static int is_id_part(unsigned char c) { return c >= 128 || isalnum(c) || c == '_'; }
static Token token(TokenType type, char *text, int line)
{
    Token t = {type, text, 0, line};
    return t;
}
static Token scan_token(Lexer *l)
{
    while (1)
    {
        unsigned char c = (unsigned char)l->source[l->current];
        if (c == ' ' || c == '\t' || c == '\r')
        {
            l->current++;
            continue;
        }
        if (c == '\n')
        {
            l->line++;
            l->current++;
            continue;
        }
        if (c == '/' && l->source[l->current + 1] == '/')
        {
            while (l->source[l->current] && l->source[l->current] != '\n')
                l->current++;
            continue;
        }
        break;
    }
    l->start = l->current;
    unsigned char c = (unsigned char)l->source[l->current++];
    if (!c)
        return token(T_EOF, xstrdup(""), l->line);
    if (is_id_start(c))
    {
        while (is_id_part((unsigned char)l->source[l->current]))
            l->current++;
        char *s = slice(l->source, l->start, l->current);
        struct
        {
            const char *word;
            TokenType type;
        } words[] = {
            {"যদি", T_IF}, {"নাহলে", T_ELSE}, {"যতক্ষণ", T_WHILE}, {"জন্য", T_FOR}, {"ফাংশন", T_FUNCTION}, {"ফেরত", T_RETURN}, {"ধরি", T_LET}, {"দেখাও", T_PRINT}, {"সত্য", T_TRUE}, {"মিথ্যা", T_FALSE}, {"শূন্য", T_NULL}, {"if", T_IF}, {"else", T_ELSE}, {"while", T_WHILE}, {"for", T_FOR}, {"function", T_FUNCTION}, {"return", T_RETURN}, {"let", T_LET}, {"print", T_PRINT}, {"true", T_TRUE}, {"false", T_FALSE}, {"null", T_NULL}, {"class", T_CLASS}, {"this", T_THIS}, {"super", T_SUPER}, {"শ্রেণী", T_CLASS}, {"এই", T_THIS}, {"সুপার", T_SUPER}};
        TokenType type = T_ID;
        size_t n = sizeof(words) / sizeof(words[0]);
        for (size_t i = 0; i < n; i++)
            if (!strcmp(s, words[i].word))
            {
                type = words[i].type;
                break;
            }
        return token(type, s, l->line);
    }
    if (isdigit(c) || (c == '.' && isdigit((unsigned char)l->source[l->current])))
    {
        while (isdigit((unsigned char)l->source[l->current]))
            l->current++;
        if (l->source[l->current] == '.')
        {
            l->current++;
            while (isdigit((unsigned char)l->source[l->current]))
                l->current++;
        }
        Token t = token(T_NUMBER, slice(l->source, l->start, l->current), l->line);
        t.number = strtod(t.text, NULL);
        return t;
    }
    if (c == '"')
    {
        while (l->source[l->current] && l->source[l->current] != '"')
        {
            if (l->source[l->current] == '\\' && l->source[l->current + 1])
                l->current++;
            if (l->source[l->current++] == '\n')
                l->line++;
        }
        if (!l->source[l->current])
            return token(T_EOF, xstrdup("unterminated string"), l->line);
        l->current++;
        char *raw = slice(l->source, l->start + 1, l->current - 1), *out = xmalloc(strlen(raw) + 1);
        size_t j = 0;
        for (size_t i = 0; raw[i]; i++)
        {
            if (raw[i] == '\\' && raw[i + 1])
            {
                char e = raw[++i];
                out[j++] = e == 'n' ? '\n' : e == 't' ? '\t'
                                                      : e;
            }
            else
                out[j++] = raw[i];
        }
        out[j] = '\0';
        free(raw);
        return token(T_STRING, out, l->line);
    }
    if (c == '&' && l->source[l->current] == '&')
    {
        l->current++;
        return token(T_AND, xstrdup("&&"), l->line);
    }
    if (c == '|' && l->source[l->current] == '|')
    {
        l->current++;
        return token(T_OR, xstrdup("||"), l->line);
    }
    if ((c == '!' || c == '=' || c == '<' || c == '>') && l->source[l->current] == '=')
    {
        char *s = slice(l->source, l->start, ++l->current);
        return token(c == '!' ? T_BANG_EQUAL : c == '=' ? T_EQUAL_EQUAL
                                           : c == '<'   ? T_LESS_EQUAL
                                                        : T_GREATER_EQUAL,
                     s, l->line);
    }
    TokenType types[256] = {['('] = T_LEFT_PAREN, [')'] = T_RIGHT_PAREN, ['{'] = T_LEFT_BRACE, ['}'] = T_RIGHT_BRACE, [','] = T_COMMA, [';'] = T_SEMICOLON, ['+'] = T_PLUS, ['-'] = T_MINUS, ['*'] = T_STAR, ['/'] = T_SLASH, ['%'] = T_PERCENT, ['.'] = T_DOT, ['!'] = T_BANG, ['='] = T_EQUAL, ['<'] = T_LESS, ['>'] = T_GREATER};
    if (types[c])
        return token(types[c], slice(l->source, l->start, l->current), l->line);
    fail_at(l->line, "unknown character '%c'", c);
    return token(T_EOF, xstrdup(""), l->line);
}

static void advance_p(Parser *p)
{
    p->previous = p->current;
    p->current = scan_token(&p->lexer);
}
static int check(Parser *p, TokenType t) { return p->current.type == t; }
static int match(Parser *p, TokenType t)
{
    if (!check(p, t))
        return 0;
    advance_p(p);
    return 1;
}
static int consume(Parser *p, TokenType t, const char *msg)
{
    if (check(p, t))
    {
        advance_p(p);
        return 1;
    }
    fail_at(p->current.line, "%s", msg);
    p->had_error = 1;
    return 0;
}
static Expr *new_expr(ExprType type, int line)
{
    Expr *e = calloc(1, sizeof(*e));
    if (!e)
        exit(1);
    e->type = type;
    e->line = line;
    return e;
}
static Stmt *new_stmt(StmtType type, int line)
{
    Stmt *s = calloc(1, sizeof(*s));
    if (!s)
        exit(1);
    s->type = type;
    s->line = line;
    return s;
}
static Expr *expression(Parser *p);
static Expr *primary(Parser *p)
{
    Token t = p->current;
    advance_p(p);
    if (t.type == T_NUMBER)
    {
        Expr *e = new_expr(E_LITERAL, t.line);
        e->literal = (Value){V_NUMBER, .number = t.number};
        return e;
    }
    if (t.type == T_STRING)
    {
        Expr *e = new_expr(E_LITERAL, t.line);
        e->literal = (Value){V_STRING, .string = t.text};
        return e;
    }
    if (t.type == T_TRUE || t.type == T_FALSE)
    {
        Expr *e = new_expr(E_LITERAL, t.line);
        e->literal = (Value){V_BOOL, .boolean = t.type == T_TRUE};
        free(t.text);
        return e;
    }
    if (t.type == T_NULL)
    {
        Expr *e = new_expr(E_LITERAL, t.line);
        e->literal = (Value){V_NULL};
        free(t.text);
        return e;
    }
    if (t.type == T_ID)
    {
        Expr *e = new_expr(E_VARIABLE, t.line);
        e->name = t.text;
        return e;
    }
    if (t.type == T_THIS)
    {
        Expr *e = new_expr(E_THIS, t.line);
        e->name = xstrdup("this");
        free(t.text);
        return e;
    }
    if (t.type == T_SUPER)
    {
        Expr *e = new_expr(E_SUPER, t.line);
        free(t.text);
        consume(p, T_DOT, "expected . after super");
        consume(p, T_ID, "expected superclass method");
        e->super.method = xstrdup(p->previous.text); free(p->previous.text);
        return e;
    }
    if (t.type == T_LEFT_PAREN)
    {
        Expr *e = expression(p);
        consume(p, T_RIGHT_PAREN, "expected ')'");
        return e;
    }
    fail_at(t.line, "expected expression");
    free(t.text);
    Expr *e = new_expr(E_LITERAL, t.line);
    e->literal = (Value){V_NULL};
    return e;
}
static Expr *unary(Parser *p)
{
    if (match(p, T_BANG) || match(p, T_MINUS))
    {
        TokenType op = p->previous.type;
        Expr *e = new_expr(E_UNARY, p->previous.line);
        e->unary.op = op;
        e->unary.right = unary(p);
        return e;
    }
    Expr *e = primary(p);
    for (;;)
    {
        if (match(p, T_LEFT_PAREN))
        {
            Expr **args = NULL;
            int n = 0;
            if (!check(p, T_RIGHT_PAREN))
                do
                {
                    args = xrealloc(args, (n + 1) * sizeof(*args));
                    args[n++] = expression(p);
                } while (match(p, T_COMMA));
            consume(p, T_RIGHT_PAREN, "expected ')' after arguments");
            Expr *c = new_expr(E_CALL, e->line);
            c->call.callee = e;
            c->call.args = args;
            c->call.count = n;
            e = c;
        }
        else if (match(p, T_DOT))
        {
            consume(p, T_ID, "expected property name");
            Expr *g = new_expr(E_GET, e->line);
            g->get.object = e;
            g->get.name = xstrdup(p->previous.text);
            free(p->previous.text);
            e = g;
        }
        else
            break;
    }
    return e;
}
static Expr *binary(Parser *p, Expr *left, TokenType *ops, int count, int logical)
{
    int found = 1;
    while (found)
    {
        found = 0;
        for (int i = 0; i < count; i++)
            if (check(p, ops[i]))
            {
                TokenType op = p->current.type;
                int line = p->current.line;
                advance_p(p);
                Expr *right = unary(p);
                while (1)
                {
                    TokenType next = p->current.type;
                    int higher = (next == T_STAR || next == T_SLASH || next == T_PERCENT);
                    if ((op == T_PLUS || op == T_MINUS) && (higher))
                        right = binary(p, right, (TokenType[]){next}, 1, 0);
                    else
                        break;
                }
                Expr *e = new_expr(logical ? E_LOGICAL : E_BINARY, line);
                e->binary.op = op;
                e->binary.left = left;
                e->binary.right = right;
                left = e;
                found = 1;
                break;
            }
    }
    return left;
}
static Expr *expression(Parser *p)
{
    Expr *left = unary(p);
    TokenType ops1[] = {T_STAR, T_SLASH, T_PERCENT};
    left = binary(p, left, ops1, 3, 0);
    TokenType ops2[] = {T_PLUS, T_MINUS};
    left = binary(p, left, ops2, 2, 0);
    TokenType ops3[] = {T_LESS, T_LESS_EQUAL, T_GREATER, T_GREATER_EQUAL, T_EQUAL_EQUAL, T_BANG_EQUAL};
    left = binary(p, left, ops3, 6, 0);
    TokenType ops4[] = {T_AND, T_OR};
    left = binary(p, left, ops4, 2, 1);
    if (match(p, T_EQUAL))
    {
        Token *dummy = &p->previous;
        (void)dummy;
        if (left->type == E_VARIABLE)
        {
            Expr *e = new_expr(E_ASSIGN, left->line);
            e->assign.name = xstrdup(left->name);
            e->assign.value = expression(p);
            left = e;
        }
        else if (left->type == E_GET)
        {
            Expr *e = new_expr(E_SET, left->line);
            e->set.object = left->get.object;
            e->set.name = xstrdup(left->get.name);
            e->set.value = expression(p);
            left = e;
        }
        else
        {
            fail_at(left->line, "invalid assignment target");
        }
    }
    return left;
}
static Stmt *statement(Parser *p);
static Stmt *block(Parser *p)
{
    Stmt *s = new_stmt(S_BLOCK, p->previous.line);
    while (!check(p, T_RIGHT_BRACE) && !check(p, T_EOF))
    {
        s->block.items = xrealloc(s->block.items, (s->block.count + 1) * sizeof(*s->block.items));
        s->block.items[s->block.count++] = statement(p);
    }
    consume(p, T_RIGHT_BRACE, "expected '}'");
    return s;
}
static Stmt *method(Parser *p, int line)
{
    Stmt *s = new_stmt(S_FUNCTION, line);
    consume(p, T_ID, "expected method name");
    s->function.name = xstrdup(p->previous.text);
    free(p->previous.text);
    consume(p, T_LEFT_PAREN, "expected '('");
    if (!check(p, T_RIGHT_PAREN))
        do
        {
            consume(p, T_ID, "expected parameter");
            s->function.params = xrealloc(s->function.params, (s->function.param_count + 1) * sizeof(char *));
            s->function.params[s->function.param_count++] = xstrdup(p->previous.text);
            free(p->previous.text);
        } while (match(p, T_COMMA));
    consume(p, T_RIGHT_PAREN, "expected ')' after parameters");
    consume(p, T_LEFT_BRACE, "expected '{'");
    s->function.body = block(p);
    return s;
}
static Stmt *statement(Parser *p)
{
    int line = p->current.line;
    if (match(p, T_LEFT_BRACE))
        return block(p);
    if (match(p, T_PRINT))
    {
        Stmt *s = new_stmt(S_PRINT, line);
        do
        {
            s->print.items = xrealloc(s->print.items, (s->print.count + 1) * sizeof(*s->print.items));
            s->print.items[s->print.count++] = expression(p);
        } while (match(p, T_COMMA));
        consume(p, T_SEMICOLON, "expected ';'");
        return s;
    }
    if (match(p, T_LET))
    {
        Stmt *s = new_stmt(S_VAR, line);
        consume(p, T_ID, "expected variable name");
        s->var.name = xstrdup(p->previous.text);
        free(p->previous.text);
        if (match(p, T_EQUAL))
            s->var.initializer = expression(p);
        else
        {
            Expr *e = new_expr(E_LITERAL, line);
            e->literal = (Value){V_NULL};
            s->var.initializer = e;
        }
        consume(p, T_SEMICOLON, "expected ';'");
        return s;
    }
    if (match(p, T_CLASS))
    {
        Stmt *s = new_stmt(S_CLASS, line);
        consume(p, T_ID, "expected class name");
        s->class_stmt.name = xstrdup(p->previous.text);
        free(p->previous.text);
        if (match(p, T_LESS))
        {
            consume(p, T_ID, "expected superclass name");
            s->class_stmt.superclass = xstrdup(p->previous.text);
            free(p->previous.text);
        }
        consume(p, T_LEFT_BRACE, "expected '{'");
        while (!check(p, T_RIGHT_BRACE) && !check(p, T_EOF))
        {
            Stmt *m = method(p, p->current.line);
            s->class_stmt.methods = xrealloc(s->class_stmt.methods, (s->class_stmt.method_count + 1) * sizeof(*s->class_stmt.methods));
            s->class_stmt.methods[s->class_stmt.method_count++] = m;
        }
        consume(p, T_RIGHT_BRACE, "expected '}' after class");
        return s;
    }
    if (match(p, T_IF))
    {
        Stmt *s = new_stmt(S_IF, line);
        consume(p, T_LEFT_PAREN, "expected '('");
        s->if_stmt.condition = expression(p);
        consume(p, T_RIGHT_PAREN, "expected ')' after condition");
        s->if_stmt.then_branch = statement(p);
        if (match(p, T_ELSE))
            s->if_stmt.else_branch = statement(p);
        return s;
    }
    if (match(p, T_WHILE))
    {
        Stmt *s = new_stmt(S_WHILE, line);
        consume(p, T_LEFT_PAREN, "expected '('");
        s->while_stmt.condition = expression(p);
        consume(p, T_RIGHT_PAREN, "expected ')' after condition");
        s->while_stmt.body = statement(p);
        return s;
    }
    if (match(p, T_FUNCTION))
        return method(p, line);
    if (match(p, T_RETURN))
    {
        Stmt *s = new_stmt(S_RETURN, line);
        s->return_stmt.value = check(p, T_SEMICOLON) ? NULL : expression(p);
        consume(p, T_SEMICOLON, "expected ';'");
        return s;
    }
    Stmt *s = new_stmt(S_EXPR, line);
    s->expression = expression(p);
    consume(p, T_SEMICOLON, "expected ';'");
    return s;
}
static Stmt *parse(const char *source)
{
    Parser p = {0};
    p.lexer.source = source;
    p.lexer.line = 1;
    p.current = scan_token(&p.lexer);
    Stmt *root = new_stmt(S_BLOCK, 1);
    while (!check(&p, T_EOF))
    {
        root->block.items = xrealloc(root->block.items, (root->block.count + 1) * sizeof(*root->block.items));
        root->block.items[root->block.count++] = statement(&p);
    }
    return p.had_error ? NULL : root;
}

static Env *env_new(Env *parent)
{
    Env *e = calloc(1, sizeof(*e));
    e->parent = parent;
    return e;
}
static int env_find(Env *e, const char *name, Value *out)
{
    for (; e; e = e->parent)
        for (int i = 0; i < e->count; i++)
            if (!strcmp(e->items[i].name, name))
            {
                *out = e->items[i].value;
                return 1;
            }
    return 0;
}
static void env_set(Env *e, const char *name, Value v)
{
    for (Env *q = e; q; q = q->parent)
        for (int i = 0; i < q->count; i++)
            if (!strcmp(q->items[i].name, name))
            {
                q->items[i].value = v;
                return;
            }
    if (e->count == e->capacity)
    {
        e->capacity = e->capacity ? e->capacity * 2 : 8;
        e->items = xrealloc(e->items, e->capacity * sizeof(*e->items));
    }
    e->items[e->count++] = (Binding){xstrdup(name), v};
}
typedef struct { int returned; Value value; } ExecResult;
static Value eval(Expr*,Env*); static ExecResult exec(Stmt*,Env*);
static int truth(Value v){return v.type==V_NULL?0:v.type==V_BOOL?v.boolean:v.type==V_NUMBER?v.number!=0:1;}
static void print_value(Value v){switch(v.type){case V_NULL:fputs("null",stdout);break;case V_NUMBER:printf("%g",v.number);break;case V_BOOL:fputs(v.boolean?"true":"false",stdout);break;case V_STRING:fputs(v.string,stdout);break;case V_CLASS:fputs(v.class_obj->name,stdout);break;case V_INSTANCE:printf("%s instance",v.instance->class_obj->name);break;default:fputs("<function>",stdout);}}
static Function *find_method(Class *c,const char*n){for(;c;c=c->superclass)for(int i=0;i<c->method_count;i++)if(!strcmp(c->methods[i]->name,n))return c->methods[i];return NULL;}
static int field_get(Instance*i,const char*n,Value*out){for(int k=0;k<i->count;k++)if(!strcmp(i->fields[k].name,n)){*out=i->fields[k].value;return 1;}return 0;}
static void field_set(Instance*i,const char*n,Value v){for(int k=0;k<i->count;k++)if(!strcmp(i->fields[k].name,n)){i->fields[k].value=v;return;}if(i->count==i->capacity){i->capacity=i->capacity?i->capacity*2:8;i->fields=xrealloc(i->fields,i->capacity*sizeof(*i->fields));}i->fields[i->count++]=(Binding){xstrdup(n),v};}
static Value call_fn(Function*f,Value recv,int n,Expr**a,Env*caller,int line){if(n!=f->param_count){fail_at(line,"function '%s' expects %d arguments",f->name,f->param_count);return(Value){V_NULL};}Env*call=env_new(f->closure);if(recv.type==V_NULL)env_find(caller,"this",&recv);if(recv.type!=V_NULL)env_set(call,"this",recv);for(int i=0;i<n;i++)env_set(call,f->params[i],eval(a[i],caller));ExecResult r=exec(f->body,call);if(f->is_initializer){Value v;if(env_find(call,"this",&v))return v;}return r.returned?r.value:(Value){V_NULL};}
static Value eval(Expr*e,Env*env){
if(e->type==E_LITERAL)return e->literal;
if(e->type==E_VARIABLE||e->type==E_THIS){Value v;if(!env_find(env,e->type==E_THIS?"this":e->name,&v)){fail_at(e->line,"unknown variable '%s'",e->type==E_THIS?"this":e->name);return(Value){V_NULL};}return v;}
if(e->type==E_ASSIGN){Value v=eval(e->assign.value,env);env_set(env,e->assign.name,v);return v;}
if(e->type==E_SET){Value o=eval(e->set.object,env),v=eval(e->set.value,env);if(o.type!=V_INSTANCE){fail_at(e->line,"only instances have fields");return(Value){V_NULL};}field_set(o.instance,e->set.name,v);return v;}
if(e->type==E_GET){Value o=eval(e->get.object,env);if(o.type!=V_INSTANCE){fail_at(e->line,"only instances have properties");return(Value){V_NULL};}Value v;if(field_get(o.instance,e->get.name,&v))return v;Function*f=find_method(o.instance->class_obj,e->get.name);if(!f){fail_at(e->line,"undefined property '%s'",e->get.name);return(Value){V_NULL};}Function*bound=calloc(1,sizeof(*bound));*bound=*f;bound->closure=env_new(f->closure);env_set(bound->closure,"this",(Value){V_INSTANCE,.instance=o.instance});return(Value){V_FUNCTION,.function=bound};}
if(e->type==E_SUPER){Value sv;if(!env_find(env,"super",&sv)||sv.type!=V_CLASS){fail_at(e->line,"super outside subclass");return(Value){V_NULL};}Function*f=find_method(sv.class_obj,e->super.method);if(!f){fail_at(e->line,"undefined superclass method '%s'",e->super.method);return(Value){V_NULL};}return(Value){V_FUNCTION,.function=f};}
if(e->type==E_UNARY){Value v=eval(e->unary.right,env);if(e->unary.op==T_BANG)return(Value){V_BOOL,.boolean=!truth(v)};if(v.type!=V_NUMBER){fail_at(e->line,"'-' needs a number");return(Value){V_NULL};}return(Value){V_NUMBER,.number=-v.number};}
if(e->type==E_LOGICAL){Value a=eval(e->binary.left,env);if(e->binary.op==T_AND&&!truth(a))return(Value){V_BOOL,.boolean=0};if(e->binary.op==T_OR&&truth(a))return(Value){V_BOOL,.boolean=1};return(Value){V_BOOL,.boolean=truth(eval(e->binary.right,env))};}
if(e->type==E_CALL){Value f=eval(e->call.callee,env);if(f.type==V_CLASS){Instance*i=calloc(1,sizeof(*i));i->class_obj=f.class_obj;Function*init=find_method(f.class_obj,"init");if(init)call_fn(init,(Value){V_INSTANCE,.instance=i},e->call.count,e->call.args,env,e->line);return(Value){V_INSTANCE,.instance=i};}if(f.type!=V_FUNCTION){fail_at(e->line,"can only call functions and classes");return(Value){V_NULL};}return call_fn(f.function,(Value){V_NULL},e->call.count,e->call.args,env,e->line);}
Value a=eval(e->binary.left,env),b=eval(e->binary.right,env);if(e->binary.op==T_PLUS&&a.type==V_STRING&&b.type==V_STRING){char*q=xmalloc(strlen(a.string)+strlen(b.string)+1);strcpy(q,a.string);strcat(q,b.string);return(Value){V_STRING,.string=q};}if(a.type!=V_NUMBER||b.type!=V_NUMBER){if(e->binary.op==T_EQUAL_EQUAL||e->binary.op==T_BANG_EQUAL){int z=a.type==b.type&&(a.type==V_NULL||(a.type==V_BOOL&&a.boolean==b.boolean)||(a.type==V_STRING&&!strcmp(a.string,b.string))||(a.type==V_INSTANCE&&a.instance==b.instance));return(Value){V_BOOL,.boolean=e->binary.op==T_EQUAL_EQUAL?z:!z};}fail_at(e->line,"operator needs numbers");return(Value){V_NULL};}double x=a.number,y=b.number;switch(e->binary.op){case T_PLUS:return(Value){V_NUMBER,.number=x+y};case T_MINUS:return(Value){V_NUMBER,.number=x-y};case T_STAR:return(Value){V_NUMBER,.number=x*y};case T_SLASH:if(!y){fail_at(e->line,"division by zero");return(Value){V_NULL};}return(Value){V_NUMBER,.number=x/y};case T_PERCENT:return(Value){V_NUMBER,.number=fmod(x,y)};case T_LESS:return(Value){V_BOOL,.boolean=x<y};case T_LESS_EQUAL:return(Value){V_BOOL,.boolean=x<=y};case T_GREATER:return(Value){V_BOOL,.boolean=x>y};case T_GREATER_EQUAL:return(Value){V_BOOL,.boolean=x>=y};case T_EQUAL_EQUAL:return(Value){V_BOOL,.boolean=x==y};default:return(Value){V_BOOL,.boolean=x!=y};}}
static ExecResult exec(Stmt*s,Env*env){ExecResult ok={0,{V_NULL}};switch(s->type){case S_EXPR:eval(s->expression,env);break;case S_PRINT:for(int i=0;i<s->print.count;i++){if(i)putchar(' ');print_value(eval(s->print.items[i],env));}putchar('\n');break;case S_VAR:env_set(env,s->var.name,eval(s->var.initializer,env));break;case S_BLOCK:for(int i=0;i<s->block.count;i++){ExecResult r=exec(s->block.items[i],env);if(r.returned)return r;}break;case S_IF:if(truth(eval(s->if_stmt.condition,env)))return exec(s->if_stmt.then_branch,env);if(s->if_stmt.else_branch)return exec(s->if_stmt.else_branch,env);break;case S_WHILE:while(truth(eval(s->while_stmt.condition,env))){ExecResult r=exec(s->while_stmt.body,env);if(r.returned)return r;}break;case S_FUNCTION:{Function*f=calloc(1,sizeof(*f));f->name=s->function.name;f->params=s->function.params;f->param_count=s->function.param_count;f->body=s->function.body;f->closure=env;env_set(env,f->name,(Value){V_FUNCTION,.function=f});break;}case S_CLASS:{Class*c=calloc(1,sizeof(*c));c->name=s->class_stmt.name;c->method_count=s->class_stmt.method_count;c->methods=calloc((size_t)c->method_count,sizeof(Function*));if(s->class_stmt.superclass){Value v;if(!env_find(env,s->class_stmt.superclass,&v)||v.type!=V_CLASS){fail_at(s->line,"undefined superclass");break;}c->superclass=v.class_obj;}for(int i=0;i<c->method_count;i++){Stmt*m=s->class_stmt.methods[i];Function*f=calloc(1,sizeof(*f));f->name=m->function.name;f->params=m->function.params;f->param_count=m->function.param_count;f->body=m->function.body;f->closure=env;if(c->superclass) { Env *method_env=env_new(env); env_set(method_env,"super",(Value){V_CLASS,.class_obj=c->superclass}); f->closure=method_env; } f->is_initializer=!strcmp(f->name,"init");c->methods[i]=f;}env_set(env,c->name,(Value){V_CLASS,.class_obj=c});break;}case S_RETURN:ok.returned=1;ok.value=s->return_stmt.value?eval(s->return_stmt.value,env):(Value){V_NULL};return ok;}return ok;}

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        fprintf(stderr, "BanglaScript: %s: %s\n", path, strerror(errno));
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    char *s = xmalloc((size_t)n + 1);
    if (fread(s, 1, (size_t)n, f) != (size_t)n)
    {
        fclose(f);
        free(s);
        return NULL;
    }
    s[n] = '\0';
    fclose(f);
    return s;
}
int bangla_run_source(const char *source)
{
    error_count = 0;
    Stmt *program = parse(source);
    if (!program)
        return 1;
    Env *global = env_new(NULL);
    exec(program, global);
    return error_count ? 1 : 0;
}
int bangla_run_file(const char *path)
{
    char *source = read_file(path);
    if (!source)
        return 1;
    int result = bangla_run_source(source);
    free(source);
    return result;
}
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#define TokenType WindowsTokenType
#include <windows.h>
#undef TokenType
#endif
static void repl(void) { char line[4096]; puts("BanglaScript 0.1 - Ctrl+D to exit"); while (1) { fputs(">>> ", stdout); fflush(stdout); if (!fgets(line, sizeof(line), stdin)) break; bangla_run_source(line); } }
int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); SetConsoleCP(CP_UTF8);
#endif
    if (argc > 2) { fprintf(stderr, "usage: %s [file.bs]\n", argv[0]); return 2; }
    if (argc == 2) return bangla_run_file(argv[1]);
    repl(); return 0;
}
