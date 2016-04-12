#ifndef _RAWSTATISTICS_H_
#define _RAWSTATISTICS_H_

#include <unordered_map>
#include <map>
#include <string>
#include <vector>
#include <cstring>

#include <skey.h>

#include <UserType.h>
#include <FilterType.h>
#include <PoliticalParty.h>

class Graph;
class StatisticsData;

struct InsensitiveCompare { 
  bool operator() (const std::string & a, const std::string & b) const {
    return strcasecmp(a.c_str(), b.c_str()) < 0;
  }
};

class RawStatistics {
 public:
  RawStatistics() {
    initialize();
  }
  
  void addUserType(UserType type) {
    user_types[type]++;
    version++;
  }

#if 0
  void addPoliticalParty(PoliticalParty party) {
    political_parties[party]++;
    version++;
  }
#endif

  void addLink(const std::string & title, const std::string & url);
  void addHashtag(const std::string & url);
  void setTimeRange(time_t t0, time_t t1) { start_time = t0; end_time = t1; }
  void setSentimentRange(float s0, float s1) { start_sentiment = s0; end_sentiment = s1; }
  void setNumRawNodes(size_t n) { num_raw_nodes = n; }
  void setNumRawEdges(size_t n) { num_raw_edges = n; }
  void setNumPosts(size_t n) { num_posts = n; }
  void setNumActiveUsers(size_t n) { num_active_users = n; }
  
  void addActivity(time_t t, short source_id, long long source_object_id, short lang, long long app_id, long long filter_id, PoliticalParty party);
  void addReceivedActivity(time_t t, short source_id, long long source_object_id, long long app_id, long long filter_id);

  void clear() {
    links.clear();
    hashtags.clear();
    headlines.clear();
    user_types.clear();
    political_parties.clear();
    hours.clear();
    weekdays.clear();
    application_usage.clear();
    filter_usage.clear();
    language_usage.clear();
    user_activity.clear();
    user_popularity.clear();

    num_raw_nodes = num_raw_edges = 0;
    num_posts = 0;
    num_active_users = 0;
    
    initialize();
  }

  time_t getStartTime() const { return start_time; }
  time_t getEndTime() const { return end_time; }
  float getStartSentiment() const { return start_sentiment; }
  float getEndSentiment() const { return end_sentiment; }
  size_t getNumRawNodes() const { return num_raw_nodes; }
  size_t getNumRawEdges() const { return num_raw_edges; }
  size_t getNumPosts() const { return num_posts; }
  size_t getNumActiveUsers() const { return num_active_users; }

  void finalize(const Graph & graph, StatisticsData & data) const;

 protected:
  void initialize() {
    for (unsigned int i = 0; i < 24; i++) hours.push_back(0);
    for (unsigned int i = 0; i < 7; i++) weekdays.push_back(0);
  }

 private:
  time_t start_time = 0, end_time = 0;
  float start_sentiment = -1, end_sentiment = 1;
  size_t num_raw_nodes = 0, num_raw_edges = 0;
  size_t num_posts = 0, num_active_users = 0;
  std::unordered_map<std::string, int> links;
  std::unordered_map<std::string, int> hashtags;
  std::unordered_map<std::string, int> headlines;
  std::map<UserType, int> user_types;
  std::map<PoliticalParty, int> political_parties;
  std::vector<unsigned int> hours, weekdays;

  std::map<skey, std::map<long long, int> > application_usage;
  std::map<skey, int> user_activity, user_popularity;

  std::map<FilterType, int> filter_usage;
  std::unordered_map<short, int> language_usage;
  
  unsigned int version = 1;
};

#endif
