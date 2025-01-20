#include "scrappers/allanime.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(stderr, "Please include title to search for\n");
    return 1;
  }
  cJSON *search_results_json = search_for_anime(
      argv[1], "dub", DEFAULT_NSFW, DEFAULT_UNKNOWN, DEFAULT_PER_PAGE,
      DEFAULT_PAGE, DEFAULT_COUNTRY_OF_ORIGIN);
  cJSON *search_results =
      cJSON_GetObjectItemCaseSensitive(search_results_json, "results");
  cJSON *anime_result;
  int index = 0;
  cJSON_ArrayForEach(anime_result, search_results) {
    index++;
    printf(
        "%i: %s\n", index,
        cJSON_GetObjectItemCaseSensitive(anime_result, "title")->valuestring);
  }
  printf("Select Search Result: ");
  scanf("%d", &index);
  cJSON *selected_anime = cJSON_GetArrayItem(search_results, index - 1);
  if (!selected_anime) {
    fprintf(stderr, "%s\n", "Invalid index : (");
    return 1;
  }
  char *anime_id =
      cJSON_GetObjectItemCaseSensitive(selected_anime, "id")->valuestring;
  cJSON *anime = get_anime(anime_id);
  printf("%s\n", cJSON_PrintUnformatted(anime));

  // clean up
  cJSON_Delete(search_results_json);
  cJSON_Delete(anime_result);
  cJSON_Delete(anime);
  free(anime_id);
  return 0;
}
