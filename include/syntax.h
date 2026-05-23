#ifndef INTERM_SYNTAX_H
#define INTERM_SYNTAX_H

#include "common.h"

typedef enum {
    IM_TOKEN_TEXT,
    IM_TOKEN_KEYWORD,
    IM_TOKEN_STRING,
    IM_TOKEN_NUMBER,
    IM_TOKEN_COMMENT,
    IM_TOKEN_OPERATOR,
    IM_TOKEN_IDENTIFIER
} im_token_type_t;

typedef struct {
    im_token_type_t type;
    size_t start;
    size_t length;
} im_token_t;

im_result_t im_syntax_tokenize_line(const char* text, size_t len, im_token_t* tokens, size_t* token_count, size_t max_tokens);

#endif // INTERM_SYNTAX_H
