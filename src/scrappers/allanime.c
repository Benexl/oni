#include "../utils/networking.h"
#include "allanime.h"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to execute a GraphQL query
cJSON *_execute_graphql_query(const char *query, cJSON *variables) {
  CURL *curl;
  CURLcode res;
  Response response = {0};

  // Initialize libcurl
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "Failed to initialize libcurl\n");
    return NULL;
  }

  // Convert variables to a JSON string
  char *variables_json = cJSON_PrintUnformatted(variables);
  if (!variables_json) {
    fprintf(stderr, "Failed to convert variables to JSON\n");
    curl_easy_cleanup(curl);
    return NULL;
  }

  char *encoded_query = curl_easy_escape(curl, query, 0);
  char *encoded_variables = curl_easy_escape(curl, variables_json, 0);

  // Prepare the URL with encoded query parameters
  char url[10024];
  snprintf(url, sizeof(url), "%s?query=%s&variables=%s", API_ENDPOINT,
           encoded_query, encoded_variables);

  // Free the encoded strings
  curl_free(encoded_query);
  curl_free(encoded_variables);

  // Set libcurl options

  struct curl_slist *headers = NULL;

  char header_buffer[256]; // Adjust the size as needed
  sprintf(header_buffer, "Referer: %s", API_REFERER);
  headers = curl_slist_append(headers, header_buffer); // Set the headers
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  // Perform the HTTP request
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "HTTP request failed: %s\n", curl_easy_strerror(res));
    free(response.data);
    free(variables_json);
    curl_easy_cleanup(curl);
    return NULL;
  }

  // Clean up libcurl
  curl_easy_cleanup(curl);
  free(variables_json);

  // Parse the JSON response
  cJSON *json_response = cJSON_Parse(response.data);
  if (!json_response) {
    fprintf(stderr, "Failed to parse JSON response\n");
    free(response.data);
    return NULL;
  }

  // Extract the "data" field from the response
  cJSON *data = cJSON_GetObjectItemCaseSensitive(json_response, "data");
  if (!data) {
    fprintf(stderr, "No 'data' field in the response\n");
    cJSON_Delete(json_response);
    free(response.data);
    return NULL;
  }

  // Clean up and return the "data" field
  free(response.data);
  return cJSON_Duplicate(data, 1); // Return a duplicate to avoid memory issues
}

cJSON *search_for_anime(const char *search_keywords,
                        const char *translation_type, int nsfw, int unknown,
                        int per_page, int page, const char *country_of_origin) {
  // Create the variables JSON object
  cJSON *variables = cJSON_CreateObject();

  // Create the search object
  cJSON *search = cJSON_CreateObject();
  cJSON_AddBoolToObject(search, "allowAdult", nsfw);
  cJSON_AddBoolToObject(search, "allowUnknown", unknown);
  cJSON_AddStringToObject(search, "query", search_keywords);

  // Add the search object and other variables
  cJSON_AddItemToObject(variables, "search", search);
  cJSON_AddNumberToObject(variables, "limit", per_page);
  cJSON_AddNumberToObject(variables, "page", page);
  cJSON_AddStringToObject(variables, "translationtype", translation_type);
  cJSON_AddStringToObject(variables, "countryorigin", country_of_origin);

  // Execute the GraphQL query
  cJSON *search_results =
      _execute_graphql_query(GQL_SEARCH_FOR_ANIME, variables);

  // Clean up the variables JSON object
  cJSON_Delete(variables);

  // Check if the query was successful
  if (!search_results) {
    fprintf(stderr, "Failed to execute GraphQL query\n");
    return NULL;
  }

  // Extract the relevant data from the response
  cJSON *shows = cJSON_GetObjectItemCaseSensitive(search_results, "shows");
  if (!shows) {
    fprintf(stderr, "No 'shows' field in the response\n");
    cJSON_Delete(search_results);
    return NULL;
  }

  cJSON *page_info = cJSON_GetObjectItemCaseSensitive(shows, "pageInfo");
  cJSON *edges = cJSON_GetObjectItemCaseSensitive(shows, "edges");

  // Create the result JSON object
  cJSON *result = cJSON_CreateObject();
  cJSON_AddItemToObject(result, "pageInfo", cJSON_Duplicate(page_info, 1));

  cJSON *results_array = cJSON_CreateArray();
  cJSON_AddItemToObject(result, "results", results_array);

  // Iterate over the edges and add each result to the array
  cJSON *edge;
  cJSON_ArrayForEach(edge, edges) {
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(
        item, "id", cJSON_GetObjectItemCaseSensitive(edge, "_id")->valuestring);
    cJSON_AddStringToObject(
        item, "title",
        cJSON_GetObjectItemCaseSensitive(edge, "name")->valuestring);
    cJSON_AddStringToObject(
        item, "type",
        cJSON_GetObjectItemCaseSensitive(edge, "__typename")->valuestring);
    cJSON_AddNumberToObject(
        item, "availableEpisodes",
        cJSON_GetObjectItemCaseSensitive(edge, "availableEpisodes")->valueint);
    cJSON_AddItemToArray(results_array, item);
  }

  // Clean up the search results JSON object
  cJSON_Delete(search_results);

  return result;
}

cJSON *get_anime(const char *id) {
  // Create variables object
  cJSON *variables = cJSON_CreateObject();
  cJSON_AddStringToObject(variables, "showId", id);

  // Execute GraphQL query
  cJSON *anime_json = _execute_graphql_query(GQL_GET_ANIME, variables);
  if (!anime_json) {
    fprintf(stderr, "Failed to get anime json\n");
    cJSON_Delete(variables); // Clean up variables
    return NULL;
  }

  // Extract the "show" object
  cJSON *show = cJSON_GetObjectItemCaseSensitive(anime_json, "show");
  if (!show) {
    fprintf(stderr, "Missing 'show' in anime json\n");
    cJSON_Delete(anime_json);
    cJSON_Delete(variables);
    return NULL;
  }

  // Create the anime object
  cJSON *anime = cJSON_CreateObject();

  // Add fields to anime object (with null checks)
  cJSON *id_item = cJSON_GetObjectItemCaseSensitive(show, "_id");
  if (id_item && id_item->valuestring) {
    cJSON_AddStringToObject(anime, "id", id_item->valuestring);
  }

  cJSON *title_item = cJSON_GetObjectItemCaseSensitive(show, "name");
  if (title_item && title_item->valuestring) {
    cJSON_AddStringToObject(anime, "title", title_item->valuestring);
  }

  cJSON *episodes_item =
      cJSON_GetObjectItemCaseSensitive(show, "availableEpisodesDetail");
  if (episodes_item) {
    cJSON_AddItemToObject(anime, "availableEpisodes",
                          cJSON_Duplicate(episodes_item, 1)); // Deep copy
  }

  cJSON *type_item = cJSON_GetObjectItemCaseSensitive(show, "__typename");
  if (type_item && type_item->valuestring) {
    cJSON_AddStringToObject(anime, "type", type_item->valuestring);
  }

  // Clean up
  cJSON_Delete(anime_json);
  cJSON_Delete(variables);

  return anime;
}
// Function to convert hex string to byte array
unsigned char *hex_to_bytes(const char *hex_string, size_t *out_length) {
  size_t len = strlen(hex_string);
  *out_length = len / 2;
  unsigned char *bytes = (unsigned char *)malloc(*out_length);

  for (size_t i = 0; i < *out_length; i++) {
    sscanf(hex_string + 2 * i, "%2hhx", &bytes[i]);
  }

  return bytes;
}

// Function to perform XOR operation and decode
char *one_digit_symmetric_xor(int password, const char *target) {
  size_t length;
  unsigned char *bytes = hex_to_bytes(target, &length);

  // Allocate memory for the result
  char *result = (char *)malloc(length + 1); // +1 for null terminator

  // Perform XOR and decode
  for (size_t i = 0; i < length; i++) {
    result[i] = bytes[i] ^ password;
  }
  result[length] = '\0'; // Null-terminate the string

  free(bytes); // Free the allocated memory for bytes
  return result;
}

// Function to replace all occurrences of "old_str" with "new_str" in a string
char *replace_string(const char *url, const char *old_str,
                     const char *new_str) {
  size_t url_length = strlen(url);
  size_t old_str_length = strlen(old_str);
  size_t new_str_length = strlen(new_str);

  // Check if old_str is empty
  if (old_str_length == 0) {
    fprintf(stderr, "Error: old_str cannot be an empty string\n");
    return NULL;
  }

  // Count the number of occurrences of "old_str"
  size_t count = 0;
  const char *pos = url;
  while ((pos = strstr(pos, old_str)) != NULL) {
    count++;
    pos += old_str_length;
  }

  // Calculate the length of the new string after replacement
  size_t new_length = url_length + count * (new_str_length - old_str_length);

  // Allocate memory for the new string (+1 for the null terminator)
  char *new_url = (char *)malloc(new_length + 1);
  if (!new_url) {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }

  // Copy the original string and replace "old_str" with "new_str"
  char *dest = new_url;
  const char *src = url;
  while (*src) {
    if (strncmp(src, old_str, old_str_length) == 0) {
      strcpy(dest, new_str);
      dest += new_str_length;
      src += old_str_length;
    } else {
      *dest++ = *src++;
    }
  }
  *dest = '\0'; // Null-terminate the new string

  return new_url;
}
cJSON *get_episode_streams(const char *id, const char *episode,
                           const char *translation_type) {
  cJSON *variables = cJSON_CreateObject();
  cJSON_AddStringToObject(variables, "showId", id);
  cJSON_AddStringToObject(variables, "translationType", translation_type);
  cJSON_AddStringToObject(variables, "episodeString", episode);

  cJSON *episode_json =
      _execute_graphql_query(GQL_GET_EPISODE_STREAMS, variables);
  if (!episode_json) {
    fprintf(stderr, "Failed to get episode info");
    cJSON_Delete(variables);
    return NULL;
  }
  episode_json = cJSON_GetObjectItemCaseSensitive(episode_json, "episode");
  cJSON *server;
  cJSON *episode_server = cJSON_CreateObject();
  cJSON_ArrayForEach(
      server, cJSON_GetObjectItemCaseSensitive(episode_json, "sourceUrls")) {
    cJSON *source_name = cJSON_GetObjectItemCaseSensitive(server, "sourceName");
    if (source_name && source_name->valuestring &&
        strcmp(source_name->valuestring, "Luf-mp4") == 0) {
      // Get the "sourceUrl" value
      cJSON *sourceUrl = cJSON_GetObjectItemCaseSensitive(server, "sourceUrl");
      if (!sourceUrl && !sourceUrl->valuestring) {
        fprintf(stderr, "sourceUrl is missing or invalid\n");
        return NULL;
      }
      const char *source_url = sourceUrl->valuestring;

      // Calculate the length of the substring to extract
      size_t substring_length = strlen(source_url + 2);

      // Allocate memory for the extracted substring (+1 for null terminator)
      char *extracted_substring = (char *)malloc(substring_length + 1);
      if (!extracted_substring) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
      }

      // Copy the substring starting from index 2
      strncpy(extracted_substring, source_url + 2, substring_length);
      extracted_substring[substring_length] = '\0'; // Null-terminate

      // Perform XOR operation
      char *decoded_url = one_digit_symmetric_xor(56, extracted_substring);

      // make request
      CURL *curl;
      CURLcode res;
      Response response = {0};
      char *url = (char *)malloc(strlen(decoded_url) + 20);

      sprintf(url, "https://%s/apivtwo/clock.json?%s", API_BASE_URL,
              decoded_url + 15);
      // Initialize libcurl
      curl_global_init(CURL_GLOBAL_DEFAULT);
      curl = curl_easy_init();
      if (!curl) {
        fprintf(stderr, "Failed to initialize libcurl\n");
        return NULL;
      }

      // Set libcurl options

      struct curl_slist *headers = NULL;

      char header_buffer[256]; // Adjust the size as needed
      sprintf(header_buffer, "Referer: %s", API_REFERER);
      headers = curl_slist_append(headers, header_buffer); // Set the headers
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

      // Perform the HTTP request
      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        fprintf(stderr, "HTTP request failed: %s\n", curl_easy_strerror(res));
        free(response.data);
        curl_easy_cleanup(curl);
        return NULL;
      }

      // Clean up libcurl
      curl_easy_cleanup(curl);

      cJSON *source_json = cJSON_Parse(response.data);

      cJSON_AddStringToObject(episode_server, "server", "gogoanime");
      cJSON *episode_headers = cJSON_CreateObject();

      cJSON_AddStringToObject(episode_headers, "Referer",
                              "https://allanime.day/");
      cJSON_AddItemToObject(episode_server, "headers", episode_headers);
      cJSON_AddArrayToObject(episode_server, "subtitles");
      char *episode_title = (char *)malloc(strlen(episode) + 8);

      sprintf(episode_title, "Episode %s", episode);
      cJSON_AddStringToObject(episode_server, "episode_title", episode_title);
      cJSON_AddItemToObject(
          episode_server, "links",
          cJSON_GetObjectItemCaseSensitive(source_json, "links"));

      // Free allocated memory
      // free(url);
      // cJSON_Delete(episode_json);
      // free(extracted_substring);
      // free(decoded_url);
      // free(response.data);
      // cJSON_Delete(source_json);
      // free(episode_title);
    }
  }
  return episode_server;
}
