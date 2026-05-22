#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

static struct termios g_orig_termios;
static bool g_raw_mode = false;

im_result_t im_terminal_init() {
    printf("  Terminal: Initializing backend...\n");
    
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) == -1) {
        return IM_ERR_IO;
    }
    
    struct termios raw = g_orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1; // 100ms timeout for read
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        return IM_ERR_IO;
    }
    
    g_raw_mode = true;
    
    // Clear screen and move cursor to home
    printf("\033[2J\033[H");
    fflush(stdout);
    
    return IM_OK;
}

im_result_t im_terminal_shutdown() {
    if (g_raw_mode) {
        printf("  Terminal: Shutting down backend...\n");
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        g_raw_mode = false;
        
        // Final reset for good measure
        printf("\033[0m"); // Reset colors
        fflush(stdout);
    }
    return IM_OK;
}

// TODO: Add im_terminal_get_size, im_terminal_write, etc.
im_result_t im_terminal_get_size(int* cols, int* rows) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return IM_ERR_IO;
    }
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return IM_OK;
}
