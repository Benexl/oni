#include <stdio.h>
#ifndef NETWORKING_H
#define NETWORKING_H

// Structure to hold the HTTP response
typedef struct {
  char *data;
  size_t size;
} Response;
size_t write_callback(void *ptr, size_t size, size_t nmemb, Response *response);
#endif
