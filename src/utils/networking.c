#include "networking.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Callback function for libcurl to write response data
size_t write_callback(void *ptr, size_t size, size_t nmemb,
                      Response *response) {
  size_t total_size = size * nmemb;
  response->data = realloc(response->data, response->size + total_size + 1);
  if (response->data == NULL) {
    fprintf(stderr, "Failed to allocate memory for response data\n");
    return 0;
  }
  memcpy(&(response->data[response->size]), ptr, total_size);
  response->size += total_size;
  response->data[response->size] = '\0';
  return total_size;
}
