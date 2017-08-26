//
#include <malloc.h>
#include <string.h>
//#include <assert.h>

char* strxcat(char *dst, const char *src) {
    size_t n;
    char *pp;
    //
    n = strlen(src);
    if (n > 0) {
        pp = realloc(dst, strlen(dst) + n + 1);
        //assert(pp != NULL);
        dst = pp;
        dst = strncat(dst, src, n);
    }
    return dst;
}
