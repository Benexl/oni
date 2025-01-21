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
  char translation_type[3];

  printf("Preferred translation type (sub/dub): ");
  scanf("%s", translation_type);
  cJSON *episodes = cJSON_GetObjectItemCaseSensitive(
      cJSON_GetObjectItemCaseSensitive(anime, "availableEpisodes"),
      translation_type);
  cJSON *episode_item;
  char episode[5];
  cJSON_ArrayForEach(episode_item, episodes) {
    printf("%s\n", episode_item->valuestring);
  }
  printf("Select Episode You Want to stream: ");
  scanf("%s", episode);
  cJSON *episode_json = get_episode_streams(anime_id, episode, "sub");
  if (!episode_json) {
    fprintf(stderr, "Failed to parse JSON\n");
    return 1;
  }

  // Extract the "links" array
  cJSON *links = cJSON_GetObjectItemCaseSensitive(episode_json, "links");
  if (!cJSON_IsArray(links)) {
    fprintf(stderr, "\"links\" is not an array\n");
    cJSON_Delete(episode_json);
    return 1;
  }

  // Get the first link in the "links" array
  cJSON *first_link = cJSON_GetArrayItem(links, 0);
  if (!first_link) {
    fprintf(stderr, "No links found\n");
    cJSON_Delete(episode_json);
    return 1;
  }

  // Extract the "link" field from the first link
  cJSON *link = cJSON_GetObjectItemCaseSensitive(first_link, "link");
  if (!cJSON_IsString(link)) {
    fprintf(stderr, "\"link\" is not a string\n");
    cJSON_Delete(episode_json);
    return 1;
  }

  // Get the URL
  const char *video_url = link->valuestring;
  printf("Extracted video URL: %s\n", video_url);

  // Construct the mpv command
  char mpv_command[512];
  snprintf(mpv_command, sizeof(mpv_command),
           "mpv \"%s\" --http-header-fields=Referer:https://allanime.day/",
           video_url);

  // Launch mpv
  printf("Launching mpv with command: %s\n", mpv_command);
  int result = system(mpv_command);

  // Check if mpv launched successfully
  if (result != 0) {
    fprintf(stderr, "Failed to launch mpv\n");
  }

  // Clean up
  cJSON_Delete(episode_json);

  // clean up
  cJSON_Delete(search_results_json);
  cJSON_Delete(anime_result);
  cJSON_Delete(anime);
  free(anime_id);
  return 0;
}
