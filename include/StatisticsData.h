#ifndef _STATISTICSDATA_H_
#define _STATISTICSDATA_H_

#include "AppRegistry.h"

#include <vector>
#include <map>

#include <UserType.h>
#include <FilterType.h>

#include "PoliticalParty.h"

struct statistics_row_s {
  short source_id;
  std::string text;
  int value;
};

class StatisticsData {
 public:
  StatisticsData() { }

  void clear() {
    sorted_hashtags.clear();
    sorted_links.clear();
    device_users.clear();
    platform_users.clear();
    top_filters.clear();
    most_active_users.clear();
    most_popular_users.clear();
    top_languages.clear();
    user_types.clear();
    top_political_parties.clear();
  }

  const std::vector<statistics_row_s> & getLinks() const { return sorted_links; }
  const std::vector<statistics_row_s> & getHashtags() const { return sorted_hashtags; }
  const std::vector<statistics_row_s> & getMostActiveUsers() const { return most_active_users; }
  const std::vector<statistics_row_s> & getMostPopularUsers() const { return most_popular_users; }
  const std::vector<std::pair<FilterType, int> > & getTopFilters() const { return top_filters; }
  const std::vector<std::pair<PoliticalParty, int> > & getTopPoliticalParties() const { return top_political_parties; }
  const std::vector<std::pair<short, int> > & getTopLanguages() const { return top_languages; }
  const std::map<UserType, int> & getUserTypes() const { return user_types; }
  const std::map<AppInfo::Device, int> & getDeviceActivity() const { return device_users; }
  const std::map<AppPlatform, int> & getPlatformActivity() const { return platform_users; }
  const std::vector<unsigned int> & getHours() const { return hours; }
  const std::vector<unsigned int> & getWeekdays() const { return weekdays; }

  // histograms (unused)
  const std::vector<unsigned int> & getNumberOfHashtags() const { return number_of_hashtags; }
  const std::vector<unsigned int> & getNumberOfLinks() const { return number_of_links; }

  int getVersion() const { return version; }
  void setVersion(int v) { version = v; }

  void setTimeRange(time_t t0, time_t t1) { start_time = t0; end_time = t1; }
  void setSentimentRange(float s0, float s1) { start_sentiment = s0; end_sentiment = s1; }

  time_t getStartTime() const { return start_time; }
  time_t getEndTime() const { return end_time; }
  float getStartSentiment() const { return start_sentiment; }
  float getEndSentiment() const { return end_sentiment; }

  std::vector<statistics_row_s> sorted_hashtags, sorted_links, most_active_users, most_popular_users;
  std::map<AppPlatform, int> platform_users;
  std::map<AppInfo::Device, int> device_users;
  std::vector<std::pair<FilterType, int> > top_filters;
  std::vector<std::pair<PoliticalParty, int> > top_political_parties;
  std::vector<std::pair<short, int> > top_languages;

  std::vector<unsigned int> hours, weekdays;
  std::map<UserType, int> user_types;
  std::map<PoliticalParty, int> political_parties;

  std::vector<unsigned int> number_of_hashtags, number_of_links;
  
 private:
  time_t start_time = 0, end_time = 0;
  float start_sentiment = -1, end_sentiment = 1;

  int version = 0;
};

#endif
