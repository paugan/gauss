#include <lexer.h>
#include <os.h>

static gauss_token_t *__gauss_lexer_read_lparen(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_rparen(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_dot(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_quote(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_backquote(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_comma(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_sharp(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_number(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_char(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_string(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_identifier(gauss_lexer_t *lexer);
static gauss_token_t *__gauss_lexer_read_vbar(gauss_lexer_t *lexer);

static gauss_sloc_t *gauss_lexer_sloc_init(
    size_t col,
    size_t off,
    size_t line,
    const char *file
) {
    gauss_sloc_t *loc;

    loc = gauss_os_calloc(sizeof(gauss_sloc_t));

    if (!loc)
        return NULL;

    loc->col = col;
    loc->off = off;
    loc->line = line;
    loc->file = gauss_os_strdup(file);
    return loc;
}

static void gauss_lexer_sloc_free(gauss_sloc_t *loc)
{
    if (!loc)
        return;

    gauss_os_free(loc, sizeof(gauss_loc_t));
    return;
}

static gauss_token_t *gauss_lexer_token_init(
    gauss_lexer_t *lexer,
    gauss_token_type_t type,
    gauss_value_t *value,
    size_t len
) {
    gauss_token_t *token;

    token = gauss_os_calloc(sizeof(gauss_token_t));

    if (!token)
        return NULL;

    token->value = value;
    token->type = type;
    token->loc.col = lexer->loc.col - len + 1;
    token->loc.off = lexer->loc.off - len;
    token->loc.line = lexer->loc.line;
    token->loc.file = lexer->file;
    token->next = NULL;
    return token;
}

static void gauss_lexer_token_free(gauss_token_t *token)
{
    if (!token)
        return;

    if (token->value)
        gauss_os_free(token->value, sizeof(gauss_value_t));

    gauss_os_free(token, sizeof(gauss_token_t));
    return;
}

static int gauss_lexer_is_digit(char c)
{
    int ret;

    ret = c >= 0x30 && c <= 0x39;
    return ret ? 0 : -1;
}

static int gauss_lexer_is_hexdigit(char c)
{
    int ret;

    ret  = c >= 0x41 && c <= 0x46;
    ret |= c >= 0x61 && c <= 0x66;
    ret |= !gauss_lexer_is_digit(c) ? 1 : 0;

    return ret ? 0 : -1;
}

static int gauss_lexer_is_alpha(char c)
{
    int ret;

    ret = (c >= 0x41 && c <= 0x5a) || (c >= 0x61 || c <= 0x7a);
    return ret ? 0 : -1;
}

static int gauss_lexer_is_alnum(char c)
{
    int ret;

    ret = !gauss_lexer_is_digit(c) || !gauss_lexer_is_alpha(c);
    return ret ? 0 : -1;
}

static int gauss_lexer_is_ws(char c)
{
    int ret;

    ret = c == ' ' || c == '\t' || c == '\n' ||
          c == '\r' || c == '\f' || c == '\v';

    return ret ? 0 : -1;
}

static int gauss_lexer_is_delim(char c)
{
    int ret;

    switch (c) {
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '\'':
        case '`':
        case ',':
        case ';':
        case '#':
        case '"':
        case '|':
            ret = 0;
            break;
        default:
            ret = -1;
            break;
    }

    return ret;
}

static int gauss_lexer_is_initial(char c)
{
    int ret;

    if (!gauss_lexer_is_alpha(c))
        return 0;

    switch (c) {
        case '!':
        case '$':
        case '%':
        case '&':
        case '*':
        case '+':
        case '-':
        case '.':
        case '/':
        case ':':
        case '<':
        case '=':
        case '>':
        case '?':
        case '@':
        case '^':
        case '_':
        case '~':
            res = 0;
            break;
        default:
            res = -1;
            break;
    }

    return res;
}

static int gauss_lexer_is_special_initial(char c)
{
    /* TODO: handle unicode char */
    return -1;
}

static int gauss_lexer_is_subseq(char c)
{
    int res;

    res = gauss_lexer_is_initial(c) ||
          gauss_lexer_is_digit(c) ||
          c == '+' || c == '-' || c == '.' || c == '@';

    return res ? 0 : -1;
}

static int gauss_lexer_is_special_subseq(char c)
{
    /* TODO: handle unicode char */
    return -1;
}

static int gauss_lexer_is_sign(char c)
{
    int ret;

    ret = c == '+' || c == '-' || c == '.';
    return ret ? 0 : -1;
}

static int gauss_lexer_is_sign_subseq(char c)
{
    int ret;

    ret = c == '+' || c == '-' || c == '@';
    return ret ? 0 : -1;
}

static void gauss_lexer_adv(gauss_lexer_t *lexer)
{
    char c;

    if (lexer->pos >= lexer->blen)
        return;

    c = lexer->buf[lexer->pos];

    lexer->pos++;
    lexer->loc.off++;

    if (c == '\n') {
        lexer->loc.line++;
        lexer->loc.col = 1;
    } else {
        lexer->loc.col++;
    }

    return;
}

static void gauss_lexer_adv_n(gauss_lexer_t *lexer, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        gauss_lexer_adv(lexer);

    return;
}

static void gauss_lexer_skip_ws(gauss_lexer_t *lexer)
{
    while (lexer->pos < lexer->blen && gauss_lexer_is_ws(lexer->buf[lexer->pos]))
        gauss_lexer_adv(lexer);

    return;
}

static void gauss_lexer_skip_lcomm(gauss_lexer_t *lexer)
{
    while (lexer->pos < lexer->blen && lexer->buf[lexer->pos] != '\n')
        gauss_lexer_adv(lexer);

    // check for trailing newline, if it exists, advance the buffer pos.
    if (lexer->pos < lexer->blen && lexer->buf[lexer->pos] == '\n')
        gauss_lexer_adv(lexer);

    return;
}

static void gauss_lexer_skip_bcomm(gauss_lexer_t *lexer)
{
    int depth;

    depth = 1;

    while (lexer->pos < lexer->blen && depth > 0) {
        if (lexer->buf[lexer->pos] == '#' &&
            lexer->pos + 1 < lexer->blen && lexer->buf[lexer->pos + 1] == '|') {
            depth++;
            gauss_lexer_adv_n(lexer, 2);
        } else if (lexer->buf[lexer->pos] == '|' &&
                   lexer->pos + 1 < lexer->blen && lexer->buf[lexer->pos + 1] == '#') {
            depth--;
            gauss_lexer_adv_n(lexer, 2);
        } else {
            gauss_lexer_adv(lexer);
        }
    }

    return;
}

static void gauss_lexer_skip_datum(gauss_lexer_t *lexer)
{
    char c;
    int depth;

    depth = 0;

    while (lexer->pos < lexer->blen) {
        c = lexer->buf[lexer->pos];

        if (c == '(' || c == '[' || c == '{') {
            depth++;
            gauss_lexer_adv(lexer);
        } else if (c == ')' || c == ']' || c == '}') {
            if (!depth)
                break;

            depth--;
            gauss_lexer_adv(lexer);
        } else if (c == '"') {
            gauss_lexer_adv(lexer);

            while (lexer->pos < lexer->blen && lexer->buf[lexer->pos] != '"') {
                if (lexer->buf[lexer->pos] == '\\')
                    gauss_lexer_adv(lexer);

                gauss_lexer_adv(lexer);
            }

            /* check for trailing string quote terminator. if it exists, skip that. */
            if (lexer->pos < lexer->blen && lexer->buf[lexer->pos] == '"')
                gauss_lexer_advance(lexer);
        } else if (c == '\'' || c == '`'') {
            /* skip single-quoted or quasiquoted datum */
            gauss_lexer_adv(lexer);
            gauss_lexer_skip_datum(lexer);
        } else if (c == ',' && lexer->pos + 1 < lexer->blen &&
                   (lexer->buf[lexer->pos + 1] == '@' || lexer->buf[lexer->pos + 1] == '.')) {
            /* skip unquoted datum */
            gauss_lexer_adv_n(lexer, 2);
            gauss_lexer_skip_datum(lexer);
        } else if (!gauss_lexer_is_ws(c)) {
            gauss_lexer_skip_ws(lexer);
        } else if (c == ';') {
            gauss_lexer_skip_lcomm(lexer);
        } else if (c == '#' &&
                   lexer->pos + 1 < lexer->blen && lexer->buf[lexer->pos + 1] == '|') {
            gauss_lexer_skip_bcomm(lexer);
        } else if (c == '\0') {
            break;
        } else {
            gauss_lexer_adv(lexer);
        }
    }

    return;
}

static void gauss_lexer_skip_directive(gauss_lexer_t *lexer)
{
    size_t start;

    start = lexer->pos;

    gauss_lexer_adv_n(lexer, 2);

    while (lexer->pos < lexer->blen &&
           gauss_lexer_is_delim(lexer->buf[lexer->pos]) < 0 && gauss_lexer_is_ws(lexer->buf[lexer->pos]))
        gauss_lexer_adv(lexer);

    return;
}

/* skip sharp atmosphere */
static void gauss_lexer_skip_shatmsp(gauss_lexer_t *lexer)
{
    if (lexer->pos + 1 < lexer->blen && lexer->buf[lexer->pos + 1] == '!')
        gauss_lexer_skip_directive(lexer);
    else if (lexer->pos + 1 < lexer->blen && lexer->buf[lexer->pos + 1] == '|')
        gauss_lexer_skip_bcomm(lexer);

    return;
}

static void gauss_lexer_skip_atmsp(gauss_lexer_t *lexer)
{
    char c, n;

    while (lexer->pos < lexer->blen) {
        c = lexer->buf[lexer->pos];

        if (!gauss_lexer_is_ws(c)) {
            gauss_lexer_skip_ws(lexer);
        } else if (c == ';') {
            gauss_lexer_skip_lcomm(lexer);
        } else if (c == '#') {
            /* only skip if it's a directive or comment */
            if (lexer->pos + 1 < lexer->blen) {
                n = lexer->buf[lexer->pos + 1];

                if (n == '!' || next == '|') {
                    gauss_lexer_skip_shatmsp(lexer);
                    continue;
                }
            }

            /* otherwise, break to let main lexer routine to handle this */
            break;
        } else {
            break;
        }
    }

    return;
}
