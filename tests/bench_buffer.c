#include "test_framework.h"
#include "buffer.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>

bool bench_buffer_performance() {
    im_buffer_t buf;
    im_buffer_init(&buf, NULL, 0);

    int num_lines = 100000;
    const char* line = "This is a sample line for benchmarking performance.\n";
    size_t line_len = strlen(line);

    clock_t start = clock();
    for (int i = 0; i < num_lines; i++) {
        im_buffer_insert(&buf, buf.total_length, line, line_len);
    }
    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Inserted %d lines in %f seconds\n", num_lines, cpu_time_used);

    ASSERT(im_buffer_get_line_count(&buf) == (size_t)num_lines + 1, "Line count mismatch");

    start = clock();
    for (int i = 0; i < 1000; i++) {
        im_buffer_insert(&buf, 500, "Extra", 5);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Performed 1000 random insertions in large buffer in %f seconds\n", cpu_time_used);

    im_buffer_destroy(&buf);
    return true;
}

int main() {
    RUN_TEST(bench_buffer_performance);
    printf("\nBenchmarks run: %d, Passed: %d, Failed: %d\n", tests_run, tests_passed, tests_failed);
    return tests_failed > 0;
}
