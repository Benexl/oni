#include <cjson/cJSON.h>
#ifndef ALLANIME_H
#define ALLANIME_H

// url endpoints
#define API_BASE_URL "allanime.day"
#define API_REFERER "https://allanime.to"
#define API_ENDPOINT "https://api.allanime.day/api"

// Define default values
#define DEFAULT_NSFW 1
#define DEFAULT_UNKNOWN 1
#define DEFAULT_PER_PAGE 40
#define DEFAULT_PAGE 1
#define DEFAULT_COUNTRY_OF_ORIGIN "all"

// graphql queries
#define GQL_SEARCH_FOR_ANIME                                                   \
  "query ( \
  $search: SearchInput \
  $limit: Int \
  $page: Int \
  $translationType: VaildTranslationTypeEnumType \
  $countryOrigin: VaildCountryOriginEnumType \
  ) { \
    shows( \
      search: $search \
      limit: $limit \
      page: $page \
      translationType: $translationType \
      countryOrigin: $countryOrigin \
    ) { \
      pageInfo { \
        total \
      } \
      edges { \
        _id \
        name \
        availableEpisodes \
        __typename \
      } \
    } \
}"

cJSON *search_for_anime(const char *search_keywords,
                        const char *translation_type, int nsfw, int unknown,
                        int limit, int page, const char *country_of_origin);
char *get_anime(const char *id, const char *translation_type);
char *get_episode_streams(const char *id, const char *episode,
                          const char *translation_type);

#endif
