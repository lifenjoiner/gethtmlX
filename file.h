//
#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>
#include <string.h>

static size_t read_file(FILE* fp, char** output) {
    size_t smart_size, count;
    size_t length = 0;
    //make it faster
    if (fp == stdin) {
        smart_size = stdin->_bufsiz;
    }
    else { //unstable for stdin!
        struct stat filestats;
        int fd = fileno(fp);
        fstat(fd, &filestats);
        smart_size = filestats.st_size + 1; // +1 to get EOF, BIG file
    }
    //
    *output = calloc(1, 1); //just in case
    while (!feof(fp)) {
        *output = realloc(*output, length + smart_size + 1);
        count = fread(*output + length, 1, smart_size, fp);
        memset(*output + length + count, 0, 1); // append 0
        length = strlen(*output);
    }
    *output = realloc(*output, length + 1);
    //
    return length;
}
