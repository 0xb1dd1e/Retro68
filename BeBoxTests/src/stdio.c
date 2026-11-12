#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
    printf("printf via newlib stdio: argc=%d\n", argc);
    fprintf(stderr, "fprintf to stderr works\n");
    fputs("fputs to stdout\n", stdout);
    char *p = malloc(64); strcpy(p, "malloc/strcpy ok"); puts(p); free(p);
    FILE *f = fopen("/tmp/gcc_stdio_test.txt", "w");
    if (!f) { puts("fopen failed"); return 1; }
    fprintf(f, "line written by newlib stdio on BeOS\n"); fclose(f);
    f = fopen("/tmp/gcc_stdio_test.txt", "r");
    char buf[80] = {0}; fgets(buf, sizeof buf, f); fclose(f);
    printf("read back: %s", buf);
    remove("/tmp/gcc_stdio_test.txt");
    return 0;
}
