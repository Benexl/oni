#include "scrappers/allanime.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(stderr, "Please include title to search for\n");
    return 1;
  }
  cJSON *search_results = search_for_anime(
      argv[1], "dub", DEFAULT_NSFW, DEFAULT_UNKNOWN, DEFAULT_PER_PAGE,
      DEFAULT_PAGE, DEFAULT_COUNTRY_OF_ORIGIN);
  printf("%s", cJSON_PrintUnformatted(search_results));
  free(search_results);
  return 0;
}
