#ifndef __GAUSS_LEXER_H
#define __GAUSS_LEXER_H

#include <os.h>

struct gauss_sloc;
struct gauss_token;
struct gauss_lexer;

typedef struct gauss_sloc gauss_sloc_t;
typedef struct gauss_token gauss_token_t;
typedef struct gauss_lexer gauss_lexer_t;

typedef enum {
    GAUSS_TOKEN_BOOL,
    GAUSS_TOKEN_NUMBER,
    GAUSS_TOKEN_CHAR,
    GAUSS_TOKEN_STRING,
    GAUSS_TOKEN_SYMBOL,
    GAUSS_TOKEN_LPAREN,
    GAUSS_TOKEN_RPAREN,
    GAUSS_TOKEN_DOT,
    GAUSS_TOKEN_QUOTE,
    GAUSS_TOKEN_BACKQUOTE,
    GAUSS_TOKEN_COMMA,
    GAUSS_TOKEN_COMMA_AT,
    GAUSS_TOKEN_SHARP,
    GAUSS_TOKEN_VECTOR_START,
    GAUSS_TOKEN_BVECTOR_START,
    GAUSS_TOKEN_LABEL_DEF,
    GAUSS_TOKEN_LABEL_REF,
    GAUSS_TOKEN_DIRECTIVE_FOLD_CASE,
    GAUSS_TOKEN_DIRECTIVE_NO_FOLD_CASE,
    GAUSS_TOKEN_EOF,
    GAUSS_TOKEN_ERROR,
    GAUSS_TOKEN_MAX
} gauss_token_type_t;

struct gauss_sloc {
    size_t col;
    size_t off;
    size_t line;
    const char *file;
};

struct gauss_token {
    gauss_sloc_t loc;
    gauss_value_t *value;
    gauss_token_type_t type;
    struct gauss_token *next;
};

struct gauss_lexer {
    size_t pos;
    size_t blen;
    const char *buf;
    const char *file;
    gauss_sloc_t loc;
    gauss_token_t *ctok;
    gauss_token_t *ptok;
};

gauss_lexer_t *gauss_lexer_init(const char *file, const char *buf, size_t blen);
void gauss_lexer_free(gauss_lexer_t *lexer);

gauss_token_t *gauss_lexer_next(gauss_lexer_t *lexer);
gauss_token_t *gauss_lexer_peek(gauss_lexer_t *lexer);
gauss_token_t *gauss_lexer_consume(gauss_lexer_t *lexer);

gauss_token_t *gauss_lexer_lex(const char *file, const char *buf, size_t blen);
gauss_token_t *gauss_lexer_make_etok(gauss_lexer_t *lexer, const char *ebuf);

const char *gauss_lexer_type_to_str(gauss_token_type_t type);

#endif /* __GAUSS_LEXER_H */
