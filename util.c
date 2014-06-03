#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>

/* ckMalloc: wrapper for malloc */
void *ckMalloc(int size) {
    void *rv = malloc(size);
    if (rv == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(2);
    }

    return rv;
}

/* ckRealloc: wrapper for realloc function */
void *ckRealloc(void *data, int size) {
    void *rv = realloc(data, size);
    if (rv == NULL) {
        fprintf(stderr, "realloc failed\n");
        exit(2);
    }

    return rv;
}


/* ckStrdup: duplicate a string */
char *ckStrdup(char *data) {
    assert(data != NULL);
    char *rv = (char *) malloc(sizeof(rv[0])*(strlen(data) + 1));
    strcpy(rv, data);
    return rv;
}

/* readLine: read a line of unbounded length from a file */
char *readLine(FILE *f) {
    int incr = 100;
    int len = incr;
    char *s = (char *) malloc(sizeof(*s)*(len+1));
    int c;

    int i = 0;
    while (((c = getc(f)) != EOF) && (c != '\n')) {
        if (i >= len) {
            len = len + incr;
            s = ckRealloc(s, sizeof(*s)*(len+1));
        }
        s[i++] = c;
    }

    s[i] = '\0';

    if ((s[0] == '\0') && (c == EOF)) {
        free(s);
        return NULL;
    }

    return s;
}
