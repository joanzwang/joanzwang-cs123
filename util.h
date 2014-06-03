#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// allocate space, fail if malloc returns NULL
void *ckMalloc(int size);

// reallocate space, fail if realloc returns NULL
void *ckRealloc(void *data, int size);

// local implentation of strdup 
char *ckStrdup(char *data);

// read a line of unbounded length from a file.  return NULL on EOF
char *readLine(FILE *f);

#endif

