#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#include "mlist.h"
#include "mstring.h"
#include "mfile.h"

#include "mvalue.h"
#include "mvm.h"

enum {
    TT_EOF,
    TT_WORD,
    TT_SPECIAL,
    TT_DIGIT,
};

struct loc_t {
    char *path;
    int line;
};

struct token_t {
    uint8_t type;
    char *data;
    int size;
    struct loc_t loc;
};

struct lexer_t {
    struct token_t *data;
    size_t size, alloc;

    struct loc_t loc;
    size_t pos;
    char *text;
};

char *get_loc(struct loc_t loc);

struct lexer_t tokenize_file(char *path);
void free_lexer(struct lexer_t *lex);
void compile(struct lexer_t *lex);

static struct token_t next_token(struct lexer_t *lex);
static void skip_space(struct lexer_t *lex);

#define CURRENT lex->text[lex->pos]
#define INBOUND (lex->pos < strlen(lex->text))

static void
skip_space(struct lexer_t *lex)
{
    // skip whitespace
    while (INBOUND && isspace(CURRENT)) {
        if (CURRENT == '\n')
            lex->loc.line++;
        lex->pos++;
    }

    // skip comments
    if (INBOUND && CURRENT == '#') {
        while (INBOUND && CURRENT != '\n')
            lex->pos++;
        skip_space(lex);
    }
}

static struct token_t
next_token(struct lexer_t *lex)
{
    struct token_t tok = {0};
    skip_space(lex);
    tok.loc = lex->loc;

    if (!INBOUND)
        return tok;

    if (isdigit(CURRENT)) {
        tok.type = TT_DIGIT;
        tok.data = &CURRENT;
        while (isdigit(CURRENT)) {
            lex->pos++;
            tok.size++;
        }
        return tok;
    }

    if (CURRENT == '.') {
        tok.type = TT_SPECIAL;
        tok.data = &CURRENT;
        tok.size = 1;
        lex->pos++;
        while (isalnum(CURRENT)) {
            lex->pos++;
            tok.size++;
        }
        return tok;
    }

    if (isalpha(CURRENT)) {
        tok.type = TT_WORD;
        tok.data = &CURRENT;
        while (isalnum(CURRENT) || CURRENT == '_') {
            lex->pos++;
            tok.size++;
        }
        return tok;
    }

    return tok;
}

#undef INBOUND
#undef CURRENT

struct lexer_t
tokenize_file(char *path)
{
    struct lexer_t lex = LIST_ALLOC(struct token_t);
    lex.loc = (struct loc_t) { path, 1 };
    lex.pos = 0;

    lex.text = read_file(path);
    if (!lex.text) {
        fprintf(stderr, "error: could not read file %s\n", path);
        exit(1);
    }

    struct token_t token = next_token(&lex);
    do {
        LIST_ADD(lex, lex.size, token);
        token = next_token(&lex);
    } while(token.type != TT_EOF);

    return lex;
}

void
free_lexer(struct lexer_t *lex)
{
    LIST_FREEP(lex);
}

#define COMPARE(tk, str) (strncmp(tk.data, str, tk.size) == 0)

static uint16_t
type_from_token(struct token_t tk)
{
    if (COMPARE(tk, ".int"))
        return TYPE_INTEGER;
    else if (COMPARE(tk, ".float"))
        return TYPE_FLOAT;

    fprintf(stderr, "%s:error: unknown type "STR_FMT"\n",
                    get_loc(tk.loc), STR_ARG(tk));
    exit(1);
}

static uint16_t
inst_from_token(struct token_t tk)
{
    if (COMPARE(tk, "halt"))
        return INST_HALT;
    else if (COMPARE(tk, "pop"))
        return INST_POP;
    else if (COMPARE(tk, "add"))
        return INST_ADD;
    else if (COMPARE(tk, "sub"))
        return INST_SUB;
    else if (COMPARE(tk, "mul"))
        return INST_MUL;
    else if (COMPARE(tk, "div"))
        return INST_DIV;

    else if (COMPARE(tk, "call"))
        return INST_CALL;
    else if (COMPARE(tk, "push"))
        return INST_PUSH;
    else if (COMPARE(tk, "move"))
        return INST_MOVE;

    fprintf(stderr, "%s:error: unknown instruction "STR_FMT"\n",
                    get_loc(tk.loc), STR_ARG(tk));
    exit(1);
}

#undef COMPARE

void
compile(struct lexer_t *lex)
{
    char out_path[1024] = {0};
    sprintf(out_path, "%s.mvm", lex->loc.path);

    char data[1024] = {0};
    size_t size = 0;
    lex->pos = 0;

    memcpy(data, ".MVM", 4);
    size += 4;

    while (lex->pos < lex->size) {
        struct token_t curr = lex->data[lex->pos];

        switch (curr.type) {
        case TT_WORD: {
            uint16_t inst = inst_from_token(curr);
            data[size+0] = inst >> 8;
            data[size+1] = inst;
            size += 2;
            lex->pos++;
        } break;
        case TT_SPECIAL: {
            uint16_t inst = type_from_token(curr);
            data[size+0] = inst >> 8;
            data[size+1] = inst;
            size += 2;
            lex->pos++;
        } break;
        case TT_DIGIT: {
            // The most beautiful piece of software ever written
            int64_t digit = strtol(curr.data, &curr.data+curr.size, 10);
            data[size+0] = digit >> (64-8);
            data[size+1] = digit >> (64-16);
            data[size+2] = digit >> (64-24);
            data[size+3] = digit >> (64-32);
            data[size+4] = digit >> (64-40);
            data[size+5] = digit >> (64-48);
            data[size+6] = digit >> (64-56);
            data[size+7] = digit;
            lex->pos++;
            size += 8;
        } break;
        default:
            fprintf(stderr, "%s:error: unknown type %d\n",
                            get_loc(curr.loc), curr.type);
            exit(1);
        }
    }

    printf("writing %d bytes to %s\n", size, out_path);
    FILE *stream = fopen(out_path, "wb");
    fwrite(data, 1, size, stream);
    fclose(stream);
}

char*
get_loc(struct loc_t loc)
{
    char str[1024] = {0};
    sprintf(str, "%s:%d", loc.path, loc.line);
    return str;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    struct lexer_t lexer = tokenize_file(argv[1]);

    // for (int i = 0; i < lexer.size; ++i) {
    //     printf("%s: %d: "STR_FMT"\n",
    //         get_loc(lexer.data[i].loc),
    //         lexer.data[i].type,
    //         STR_ARG(lexer.data[i]));
    // }

    compile(&lexer);
    free_lexer(&lexer);
    return 0;
}
