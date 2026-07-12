#include <lexer.h>
#include <os.h>

static gauss_token_t *gauss_lexer_read_lparen(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_rparen(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_dot(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_quote(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_backquote(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_comma(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_sharp(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_number(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_char(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_string(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_identifier(gauss_lexer_t *lexer);
static gauss_token_t *gauss_lexer_read_vbar(gauss_lexer_t *lexer);

/* check if char is a number suffix */
static int gauss_lexer_is_nsuff(char c)
{
    int ret;

    switch (c) {
        case 'e':
        case 'E':
        case 's':
        case 'S':
        case 'f':
        case 'F':
        case 'd':
        case 'D':
        case 'l':
        case 'L':
            ret = 0;
            break;
        default:
            ret = -1;
            break;
    }

    return ret;
}

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

static void gauss_lexer_rev(gauss_lexer_t *lexer)
{
    char c;

    if (lexer->pos <= 0)
        return;

    c = lexer->buf[lexer->pos];

    lexer->pos--;
    lexer->loc.off--;

    if (c == '\n') {
        lexer->loc.line--;
        lexer->loc.col = lexer->loc.off;
    } else {
        lexer->loc.col--;
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

static void gauss_lexer_rev_n(gauss_lexer_t *lexer, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        gauss_lexer_rev(lexer);

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

/* skip atmospheres (whitespaces, comments, directives) */
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

                if (n == '!' || n == '|') {
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

static gauss_token_t *gauss_lexer_read_lparen(gauss_lexer_t *lexer)
{
    gauss_token_t *ret;

    gauss_lexer_adv(lexer);

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_LPAREN, NULL, 1);

    if (!ret)
        return NULL;

    return ret;
}

static gauss_token_t *gauss_lexer_read_rparen(gauss_lexer_t *lexer)
{
    gauss_token_t *ret;

    gauss_lexer_adv(lexer);

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_RPAREN, NULL, 1);

    if (!ret)
        return NULL;

    return ret;
}

static gauss_token_t *gauss_lexer_read_dot(gauss_lexer_t *lexer)
{
    gauss_token_t *ret;

    /* check if it's a number like .123 */
    if (lexer->pos + 1 < lexer->blen && !gauss_lexer_is_digit(lexer->buf[lexer->pos + 1]))
        return gauss_lexer_read_number(lexer);

    gauss_lexer_adv(lexer);

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_DOT, NULL, 1);

    if (!ret)
        return NULL;

    return ret;
}

static gauss_token_t *gauss_lexer_read_quote(gauss_lexer_t *lexer)
{
    gauss_token_t *ret;

    gauss_lexer_adv(lexer);

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_QUOTE, NULL, 1);

    if (!ret)
        return NULL;

    return ret;
}

static gauss_token_t *gauss_lexer_read_backquote(gauss_lexer_t *lexer)
{
    gauss_token_t *ret;

    gauss_lexer_adv(lexer);

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_BACKQUOTE, NULL, 1);

    if (!ret)
        return NULL;

    return ret;
}

static gauss_token_t *gauss_lexer_read_comma(gauss_lexer_t *lexer)
{
    gauss_token_t *ret;

    gauss_lexer_adv(lexer);

    if (lexer->pos < lexer->blen && lexer->buf[lexer->pos] == '@') {
        gauss_lexer_adv(lexer);

        ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_COMMA_AT, NULL, 2);

        if (!ret)
            return NULL;

        return ret;
    }

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_COMMA, NULL, 1);

    if (!ret)
        return NULL;

    return ret;
}

static gauss_token_t *gauss_lexer_read_sharp(gauss_lexer_t *lexer)
{
    char c;
    gauss_token_t *ret;

    ret = NULL;

    if (lexer->pos + 1 >= lexer->blen) {
        ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_ERROR, NULL, 1);
        goto __fallback;
    }

    c = lexer->buf[lexer->pos + 1];

    gauss_lexer_adv_n(lexer, 2);

    switch (c) {
        case 't':
        case 'T':
            ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_BOOL, gauss_lexer_bool_true(), 2);
            break;
        case 'f':
        case 'F':
            ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_BOOL, gauss_lexer_bool_false(), 2);
            break;
        case '\\':
            ret = gauss_lexer_read_char(lexer);
            break;
        case '(':
            ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_VECTOR_START, NULL, 2);
            break;
        case 'u':
            if (lexer->pos < lexer->blen && lexer->buf[lexer->pos] == '8') {
                gauss_lexer_adv(lexer);

                if (lexer->pos < lexer->blen && lexer->buf[lexer->pos] == '(') {
                    gauss_lexer_adv(lexer);

                    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_BVECTOR_START, NULL, 4);
                    break;
                }

                gauss_lexer_adv(lexer);
            }

            break;
        default:
            break;
    }

    if (!ret)
        ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_ERROR, NULL, 1);

__fallback:
    return ret;
}

static gauss_token_t *gauss_lexer_read_char(gauss_lexer_t *lexer)
{
    int rev;
    uint32_t cp;
    gauss_token_t *ret;
    gauss_value_t *cval;

    rev = 0;
    ret = NULL;

    if (lexer->pos >= lexer->blen) {
        if (lexer->pos + 5 <= lexer->blen && !strncmp(lexer->buf + lexer->pos, "space", 5)) {
            cp = 0x20;
            rev = 5;
        } else if (lexer->pos + 7 <= lexer->blen && !strncmp(lexer->buf + lexer->pos, "newline", 7)) {
            cp = 0x0a;
            rev = 7;
        } else if (lexer->pos + 3 <= lexer->blen && !strncmp(lexer->buf + lexer->pos, "tab", 3)) {
            cp = 0x09;
            rev = 3;
        } else if (lexer->pos + 6 <= lexer->blen && !strncmp(lexer->buf + lexer->pos, "return", 6)) {
            cp = 0x0d;
            rev = 6;
        } else if (lexer->pos + 5 <= lexer->blen && !strncmp(lexer->buf + lexer->pos, "alarm", 5)) {
            cp = 0x07;
            rev = 5;
        } else {
            cp = c;
            rev = 1;
        }
    } else {
        cp = c;
        rev = 1;
    }

    gauss_lexer_adv_n(lexer, rev);

    cval = gauss_lexer_chinit(cp);

    if (!cval) {
        gauss_lexer_rev_n(lexer, rev);
        return NULL;
    }

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_CHAR, cval, lexer->loc.off + 2 - lexer->loc.off);

    if (!ret) {
        gauss_lexer_rev_n(lexer, rev);
        gauss_lexer_chfree(cval);
        return NULL;
    }

    return ret;
}

static gauss_token_t *gauss_lexer_read_string(gauss_lexer_t *lexer)
{
    char c;
    size_t st, sst, len;
    uint32_t cp;
    gauss_token_t *ret;
    gauss_value_t *vstr;

    cp = 0;
    st = lexer->pos;
    sst = lexer->pos + 1;

    gauss_lexer_adv(lexer);

    while (lexer->pos < lexer->blen && lexer->buf[lexer->pos] != '"') {
        if (lexer->buf[lexer->pos] == '\\') {
            /* skip backslash */
            gauss_lexer_adv(lexer);

            if (lexer->pos < lexer->blen) {
                /* handle escape sequence */
                c = lexer->buf[lexer->pos];

                switch (c) {
                    case 'a':
                        cp = 0x07;
                        break;
                    case 'b':
                        cp = 0x08;
                        break;
                    case 't':
                        cp = 0x09;
                        break;
                    case 'n':
                        cp = 0x0a;
                        break;
                    case 'r':
                        cp = 0x0d;
                        break;
                    case '"':
                        cp = '"';
                        break;
                    case '\\':
                        cp = '\\';
                        break;
                    case '|':
                        cp = '|';
                        break;
                    default:
                        cp = c;
                        break;
                }

                gauss_lexer_adv(lexer);
            }
        } else {
            gauss_lexer_adv(lexer);
        }
    }

    GAUSS_UNUSED(cp);

    if (lexer->pos >= lexer->blen) {
        ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_ERROR, NULL, lexer->pos - st + 1);

        if (!ret)
            return NULL;

        return ret;
    }

    len = lexer->pos - sst;
    vstr = gauss_lexer_strinit(lexer->input + sst, len);

    if (!vstr)
        return NULL;

    gauss_lexer_adv(lexer);

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_STRING, vstr, lexer->pos - st + 1);

    if (!ret)
        return NULL;

    return ret;
}

static gauss_token_t *gauss_lexer_read_number(gauss_lexer_t *lexer)
{
    int rad, hsuf;
    int hdig, hdot;
    char c;
    char *vbuf, *eptr;
    long dnum;
    double fnum;
    size_t st, len;
    gauss_token_t *ret;
    gauss_value_t *vnum;

    st = lexer->pos;
    rad = 10;
    vnum = NULL;

    /* check for explicit sign */
    if (lexer->pos < lexer->blen &&
        (lexer->buf[lexer->pos] == '+' || lexer->buf[lexer->pos] == '-'))
        gauss_lexer_adv(lexer);

    /* check for radix prefix */
    if (lexer->pos < lexer->blen && lexer->buf[lexer->pos] == '#') {
        gauss_lexer_adv(lexer);

        if (lexer->pos < lexer->blen) {
            switch (lexer->buf[lexer->pos]) {
                case 'b':
                    rad = 2;
                    break;
                case 'o':
                    rad = 8;
                    break;
                case 'd':
                    rad = 10;
                    break;
                case 'x':
                    rad = 16;
                    break;
                default:
                    break;
            }

            gauss_lexer_adv(lexer);
        }
    }

    hdig = 0;
    hdot = 0;

    while (lexer->pos < lexer->blen) {
        c = lexer->buf[lexer->pos];

        if (c == '.') {
            if (hdot)
                break;

            hdot = 1;
            gauss_lexer_adv(lexer);
        } else if (!gauss_lexer_is_digit(c) || (rad == 16 && !gauss_lexer_is_hexdigit(c))) {
            hdig = 1;
            gauss_lexer_adv(lexer);
        } else if (!gauss_lexer_is_nsuff(c)) {
            if (hdig)
                gauss_lexer_adv(lexer);
            else
                break;
        } else if (c == '+' || c == '-') {
            /* exponent sign */
            if (hdig && hdot)
                gauss_lexer_adv(lexer);
            else
                break;
        } else {
            break;
        }
    }

    if (!hdig) {
        ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_ERROR, NULL, lexer->pos - st);

        if (!ret)
            return NULL;

        return ret;
    }

    len = lexer->pos - st;
    vbuf = gauss_os_calloc(len + 1);

    if (!vbuf) {
        ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_ERROR, NULL, len);

        if (!ret)
            return NULL;

        return ret;
    }

    /* most of the platforms have the same interface to memcpy() */
    memcpy(vbuf, lexer->buf + st, len);

    if (hdot) {
        /* parse as float */
        fnum = strtod(vbuf, &eptr);

        if (eptr != vbuf)
            vnum = gauss_lexer_real_inexact_init(fnum);
    } else {
        /* parse as integer */
        dnum = strtol(vbuf, &eptr, rad);

        if (eptr != vbuf)
            vnum = gauss_lexer_dec_exact_init(dnum);
    }

    gauss_os_free(vbuf, len + 1);

    if (!vnum) {
        ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_ERROR, NULL, len);

        if (!ret)
            return NULL;

        return ret;
    }

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_NUMBER, vnum, len);

    if (!ret)
        return NULL;

    return ret;
}

static gauss_token_t *gauss_lexer_read_identifier(gauss_lexer_t *lexer)
{
    char c;
    char *vbuf;
    size_t st, len;
    gauss_token_t *ret;
    gauss_value_t *vid;

    if (!gauss_lexer_is_initial(lexer->buf[lexer->pos]) ||
        !gauss_lexer_is_special_intiial(lexer->buf[lexer->pos])) {
        gauss_lexer_adv(lexer);
    } else if (!gauss_lexer_is_sign(lexer->buf[lexer->pos])) {
        /* could be a number / symbol */
        if (lexer->pos + 1 < lexer->blen && !gauss_lexer_is_digit(lexer->buf[lexer->pos + 1])) {
            ret = gauss_lexer_read_number(lexer);

            if (!ret)
                return NULL;

            return ret;
        }

        gauss_lexer_adv(lexer);
    } else {
        ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_ERROR, NULL, 1);

        if (!ret)
            return NULL;

        return ret;
    }

    /* read subsequent chars */
    while (lexer->pos < lexer->blen) {
        c = lexer->buf[lexer->pos];

        if (!gauss_lexer_is_subseq(c) ||
            !gauss_lexer_is_special_subseq(c) || !gauss_lexer_is_sign_subseq(c))
            gauss_lexer_adv(lexer);
        else
            break;
    }

    len = lexer->pos - start;
    vbuf = gauss_os_calloc(len + 1);

    if (!vbuf) {
        ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_ERROR, NULL, len);

        if (!ret)
            return NULL;

        return ret;
    }

    memcpy(vbuf, lexer->buf + st, len);

    vid = gauss_lexer_sym_init_cbuf(vbuf);

    if (!vid) {
        gauss_os_free(vbuf, len + 1);
        return NULL;
    }

    gauss_os_free(vbuf, len + 1);

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_SYMBOL, vid, len);

    if (!ret)
        return NULL;

    return ret;
}

static gauss_token_t *gauss_lexer_read_vbar(gauss_lexer_t *lexer)
{
    gauss_token_t *ret;

    gauss_lexer_adv(lexer);

    ret = gauss_lexer_token_init(lexer, GAUSS_TOKEN_ERROR, NULL, 1);

    if (!ret)
        return NULL;

    return ret;
}

gauss_token_t *gauss_lexer_next(gauss_lexer_t *lexer)
{
    char c;
    gauss_token_t *ret;

    gauss_lexer_skip_atmsp(lexer);

    if (lexer->pos >= lexer->blen)
        return gauss_lexer_token_init(lexer, GAUSS_TOKEN_EOF, NULL, 1);

    c = lexer->buf[lexer->pos];

    switch (c) {
        case '(':
            ret = gauss_lexer_read_lparen(lexer);
            break;
        case ')':
            ret = gauss_lexer_read_rparen(lexer);
            break;
        case '.':
            ret = gauss_lexer_read_dot(lexer);
            break;
        case '\'':
            ret = gauss_lexer_read_quote(lexer);
            break;
        case '`':
            ret = gauss_lexer_read_backquote(lexer);
            break;
        case ',':
            ret = gauss_lexer_read_comma(lexer);
            break;
        case '#':
            ret = gauss_lexer_read_sharp(lexer);
            break;
        case '"':
            ret = gauss_lexer_read_string(lexer);
            break;
        case '|':
            ret = gauss_lexer_read_vbar(lexer);
            break;
        default:
            if (!gauss_lexer_is_digit(c)) {
                ret = gauss_lexer_read_number(lexer);
            } else if (!gauss_lexer_is_sign(c) &&
                       lexer->pos + 1 < lexer->blen && !gauss_lexer_is_digit(lexer->buf[lexer->pos + 1])) {
                ret = gauss_lexer_read_number(lexer);
            } else if (!gauss_lexer_is_initial(c) || !gauss_lexer_is_special_initial(c)) {
                ret = gauss_lexer_read_identifier(lexer);
            } else if (!gauss_lexer_is_sign(c)) {
                ret = gauss_lexer_read_identifier(lexer);
            } else {
                gauss_lexer_adv(lexer);
                ret = token_create(lexer, GAUSS_TOKEN_ERROR, NULL, 1);
            }

            break;
    }

    return ret;
}

gauss_lexer_t *gauss_lexer_init(const char *file, const char *buf, size_t blen)
{
    gauss_lexer_t *ret;

    ret = gauss_os_calloc(sizeof(gauss_lexer_t));

    if (!ret)
        return NULL;

    ret->buf = buf;
    ret->blen = blen;
    ret->pos = 0;
    ret->file = gauss_os_strdup(file);
    ret->loc.file = ret->file;
    ret->loc.line = 1;
    ret->loc.col = 1;
    ret->loc.off = 0;
    ret->ctok = NULL;
    ret->ptok = NULL;

    return ret;
}

void gauss_lexer_free(gauss_lexer_t *lexer)
{
    if (!lexer)
        return;

    if (lexer->file) {
        gauss_os_free(lexer->file, strlen(lexer->file));
        lexer->file = NULL;
        lexer->loc.file = NULL;
    }

    if (lexer->ctok)
        gauss_lexer_token_free(lexer->ctok);

    if (lexer->ptok)
        gauss_lexer_token_free(lexer->ptok);

    gauss_os_free(lexer, sizeof(gauss_lexer_t));
    return;
}

gauss_token_t *gauss_lexer_peek(gauss_lexer_t *lexer)
{
    if (lexer->ptok)
        return lexer->ptok;

    lexer->ptok = gauss_lexer_next(lexer);
    return lexer->ptok;
}

gauss_token_t *gauss_lexer_consume(gauss_lexer_t *lexer)
{
    gauss_token_t *ret;

    if (lexer->ptok) {
        ret = lexer->ptok;
        lexer->ptok = NULL;

        if (lexer->ctok)
            gauss_lexer_token_free(lexer->ctok);

        lexer->ctok = ret;
        return ret;
    }

    if (lexer->ctok)
        gauss_lexer_token_free(lexer->ctok);

    lexer->ctok = gauss_lexer_next(lexer);
    return lexer->ctok;
}

gauss_token_t *gauss_lexer_lex(const char *file, const char *buf, size_t blen)
{
    gauss_lexer_t *lex;
    gauss_token_t *fptr, *tptr;
    gauss_token_t **lptr;

    lex = gauss_lexer_init(file, buf, blen);

    if (!lex)
        return NULL;

    fptr = NULL;
    lptr = &fptr;

    do {
        tptr = gauss_lexer_next(lex);

        if (tptr) {
            *lptr = tptr;
            lptr = &tptr->next;
        }
    } while (tptr && tptr->type != GAUSS_TOKEN_EOF);

    gauss_lexer_free(lex);
    return fptr;
}

#ifdef GAUSS_DEBUG

const char *gauss_lexer_type_to_str(gauss_token_type_t type)
{
    switch (type) {
        case GAUSS_TOKEN_BOOL:
            return "BOOL";
        case GAUSS_TOKEN_NUMBER:
            return "NUMBER";
        case GAUSS_TOKEN_CHAR:
            return "CHAR";
        case GAUSS_TOKEN_STRING:
            return "STRING";
        case GAUSS_TOKEN_SYMBOL:
            return "SYMBOL";
        case GAUSS_TOKEN_LPAREN:
            return "LPAREN";
        case GAUSS_TOKEN_RPAREN:
            return "RPAREN";
        case GAUSS_TOKEN_DOT:
            return "DOT";
        case GAUSS_TOKEN_QUOTE:
            return "QUOTE";
        case GAUSS_TOKEN_BACKQUOTE:
            return "BACKQUOTE";
        case GAUSS_TOKEN_COMMA:
            return "COMMA";
        case GAUSS_TOKEN_COMMA_AT:
            return "COMMA_AT";
        case GAUSS_TOKEN_SHARP:
            return "SHARP";
        case GAUSS_TOKEN_VECTOR_START:
            return "VECTOR_START";
        case GAUSS_TOKEN_BVECTOR_START:
            return "BVECTOR_START";
        case GAUSS_TOKEN_LABEL_DEF:
            return "LABEL_DEF";
        case GAUSS_TOKEN_LABEL_REF:
            return "LABEL_REF";
        case GAUSS_TOKEN_DIRECTIVE_FOLD_CASE:
            return "DIRECTIVE_FOLD_CASE";
        case GAUSS_TOKEN_DIRECTIVE_NO_FOLD_CASE:
            return "DIRECTIVE_NO_FOLD_CASE";
        case GAUSS_TOKEN_EOF:
            return "EOF";
        case GAUSS_TOKEN_ERROR:
            return "ERROR";
    }

    return "UNDEFINED";
}

#endif
