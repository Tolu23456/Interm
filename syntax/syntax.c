#include "syntax.h"
#include <string.h>
#include <ctype.h>

static bool is_keyword(const char* word, size_t len) {
    const char* keywords[] = {"if", "else", "for", "while", "return", "int", "char", "void", "static", "struct", "enum", "include", "define"};
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        if (strlen(keywords[i]) == len && strncmp(keywords[i], word, len) == 0) return true;
    }
    return false;
}

im_result_t im_syntax_tokenize_line(const char* text, size_t len, im_token_t* tokens, size_t* token_count, size_t max_tokens) {
    size_t count = 0;
    size_t i = 0;
    while (i < len && count < max_tokens) {
        if (isspace(text[i])) {
            i++;
            continue;
        }
        if (text[i] == '/' && i + 1 < len && text[i+1] == '/') {
            tokens[count++] = (im_token_t){IM_TOKEN_COMMENT, i, len - i};
            break;
        }
        if (text[i] == '"') {
            size_t start = i; i++;
            while (i < len && text[i] != '"') i++;
            if (i < len) i++;
            tokens[count++] = (im_token_t){IM_TOKEN_STRING, start, i - start};
            continue;
        }
        if (isdigit(text[i])) {
            size_t start = i;
            while (i < len && isdigit(text[i])) i++;
            tokens[count++] = (im_token_t){IM_TOKEN_NUMBER, start, i - start};
            continue;
        }
        if (isalpha(text[i]) || text[i] == '_') {
            size_t start = i;
            while (i < len && (isalnum(text[i]) || text[i] == '_')) i++;
            im_token_type_t type = is_keyword(text + start, i - start) ? IM_TOKEN_KEYWORD : IM_TOKEN_IDENTIFIER;
            tokens[count++] = (im_token_t){type, start, i - start};
            continue;
        }
        if (ispunct(text[i])) {
            tokens[count++] = (im_token_t){IM_TOKEN_OPERATOR, i, 1};
            i++;
            continue;
        }
        i++;
    }
    *token_count = count;
    return IM_OK;
}
