#include <stdio.h>
#include <stdlib.h>
static void bye(void){ printf("atexit B"); }
static void bye2(void){ printf("atexit A, "); }
int main(void){ atexit(bye); atexit(bye2); printf("main done, no newline, "); return 0; }
