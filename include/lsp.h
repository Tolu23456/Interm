#ifndef INTERM_LSP_H
#define INTERM_LSP_H

#include "common.h"

im_result_t im_lsp_init();
im_result_t im_lsp_shutdown();
im_result_t im_lsp_start_server(const char* name, const char* cmd);

#endif // INTERM_LSP_H
