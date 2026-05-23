#include "test_framework.h"
#include "buffer.h"
#include <string.h>
#include <stdlib.h>

bool test_buffer_init() {
    im_buffer_t buf;
    const char* initial = "Hello World";
    ASSERT(im_buffer_init(&buf, initial, strlen(initial)) == IM_OK, "Buffer init failed");
    ASSERT(buf.total_length == strlen(initial), "Total length mismatch");

    char* text = im_buffer_get_range(&buf, 0, buf.total_length);
    ASSERT(text != NULL, "get_range returned NULL");
    ASSERT(strcmp(text, initial) == 0, "Buffer content mismatch");

    free(text);
    im_buffer_destroy(&buf);
    return true;
}

bool test_buffer_insert() {
    im_buffer_t buf;
    im_buffer_init(&buf, "Hello World", 11);

    ASSERT(im_buffer_insert(&buf, 5, " Big", 4) == IM_OK, "Insert failed");
    ASSERT(buf.total_length == 15, "Length after insert mismatch");

    char* text = im_buffer_get_range(&buf, 0, buf.total_length);
    ASSERT(strcmp(text, "Hello Big World") == 0, "Content after insert mismatch");

    free(text);
    im_buffer_destroy(&buf);
    return true;
}

bool test_buffer_delete() {
    im_buffer_t buf;
    im_buffer_init(&buf, "Hello Big World", 15);

    ASSERT(im_buffer_delete(&buf, 5, 4) == IM_OK, "Delete failed");
    ASSERT(buf.total_length == 11, "Length after delete mismatch");

    char* text = im_buffer_get_range(&buf, 0, buf.total_length);
    ASSERT(strcmp(text, "Hello World") == 0, "Content after delete mismatch");

    free(text);
    im_buffer_destroy(&buf);
    return true;
}

bool test_buffer_line_indexing() {
    im_buffer_t buf;
    const char* content = "Line 1\nLine 2\nLine 3";
    im_buffer_init(&buf, content, strlen(content));

    ASSERT(im_buffer_get_line_count(&buf) == 3, "Line count mismatch");
    ASSERT(buf.line_offsets[0] == 0, "Line 0 offset mismatch");
    ASSERT(buf.line_offsets[1] == 7, "Line 1 offset mismatch");
    ASSERT(buf.line_offsets[2] == 14, "Line 2 offset mismatch");

    im_buffer_insert(&buf, 7, "Inserted\n", 9);
    ASSERT(im_buffer_get_line_count(&buf) == 4, "Line count mismatch after insert");
    ASSERT(buf.line_offsets[1] == 7, "Line 1 offset mismatch after insert");
    ASSERT(buf.line_offsets[2] == 16, "Line 2 offset mismatch after insert");

    im_buffer_destroy(&buf);
    return true;
}

bool test_buffer_undo_redo() {
    im_buffer_t buf;
    im_buffer_init(&buf, "Hello", 5);

    im_buffer_insert(&buf, 5, " World", 6);
    char* text = im_buffer_get_range(&buf, 0, buf.total_length);
    ASSERT(strcmp(text, "Hello World") == 0, "Insert failed");
    free(text);

    im_buffer_undo(&buf);
    text = im_buffer_get_range(&buf, 0, buf.total_length);
    ASSERT(strcmp(text, "Hello") == 0, "Undo failed");
    free(text);

    im_buffer_redo(&buf);
    text = im_buffer_get_range(&buf, 0, buf.total_length);
    ASSERT(strcmp(text, "Hello World") == 0, "Redo failed");
    free(text);

    im_buffer_destroy(&buf);
    return true;
}

int main() {
    RUN_TEST(test_buffer_init);
    RUN_TEST(test_buffer_insert);
    RUN_TEST(test_buffer_delete);
    RUN_TEST(test_buffer_line_indexing);
    RUN_TEST(test_buffer_undo_redo);

    printf("\nTests run: %d, Passed: %d, Failed: %d\n", tests_run, tests_passed, tests_failed);
    return tests_failed > 0;
}
