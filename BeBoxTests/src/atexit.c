#include <stdio.h>
#include <stdlib.h>
static void bye(void){ printf("atexit handler ran\n"); }
int main(void){ atexit(bye); printf("main done\n"); return 0; }
