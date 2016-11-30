#include "RawStatistics.h"

#include <StringUtils.h>
#include "StatisticsData.h"
#include "AppRegistry.h"

#include <DateTime.h>
#include <Graph.h>
#include <URI.h>

#include <algorithm>
#include <iostream>
#include <cassert>

using namespace std;

static bool compareQuantity1(const pair<string, int> & a, const pair<string, int> & b) {
  return a.second > b.second;
}

static bool compareQuantity3(const pair<skey, int> & a, const pair<skey, int> & b) {
  return a.second > b.second;
}

void
RawStatistics::finalize(const Graph & graph, StatisticsData & output) const {
  if (version == output.getVersion()) return;
  output.setVersion(version);
  output.clear();

  output.setTimeRange(start_time, end_time);
  output.setSentimentRange(start_sentiment, end_sentiment);

  vector<pair<string, int> > tmp_hashtags, tmp_links;
  
  for (auto & p : hashtags) tmp_hashtags.push_back(p);
  for (auto & p : links) tmp_links.push_back(p);
  
  sort(tmp_hashtags.begin(), tmp_hashtags.end(), compareQuantity1);
  sort(tmp_links.begin(), tmp_links.end(), compareQuantity1);

  map<string, pair<string, int> > tmp_hashtags2, tmp_links2;
  for (auto & p : tmp_hashtags) {
    string lc = StringUtils::toLower(p.first);
    auto it = tmp_hashtags2.find(lc);
    if (it != tmp_hashtags2.end()) {
      it->second.second += p.second;
    } else {
      tmp_hashtags2[lc] = p;
    }
  }
  for (auto & p : tmp_links) {
    string lc = StringUtils::toLower(p.first);
    auto it = tmp_links2.find(lc);
    if (it != tmp_links2.end()) {
      it->second.second += p.second;
    } else {
      tmp_links2[lc] = p;
    }
  }

  for (auto & p : tmp_hashtags2) output.sorted_hashtags.push_back( { 0, p.second.first, p.second.second } );
  for (auto & p : tmp_links2) output.sorted_links.push_back( { 0, p.second.first, p.second.second } );

  vector<pair<skey, int> > most_active_users0;
  for (auto & ua : user_activity) {
    most_active_users0.push_back(pair<skey, int>(ua.first, ua.second));
  }
  sort(most_active_users0.begin(), most_active_users0.end(), compareQuantity3);
  while (most_active_users0.size() > 25) most_active_users0.pop_back();

  if (!most_active_users0.empty()) {
    const table::Column & username = graph.getNodeArray().getTable()["uname"];
    for (auto & ud : most_active_users0) {
      int i = graph.getNodeArray().getNodeId(ud.first.source_id, ud.first.source_object_id);
      string uname;
      if (i == -1) {
	uname = "(not available)";
      } else {
	uname = "@" + username.getText(i);
      }
      output.most_active_users.push_back({ ud.first.source_id, uname, ud.second });
    }
  }

  vector<pair<skey, int> > most_popular_users0;
  for (auto & ua : user_popularity) {
    most_popular_users0.push_back(pair<skey, int>(ua.first, ua.second));
  }
  sort(most_popular_users0.begin(), most_popular_users0.end(), compareQuantity3);
  while (most_popular_users0.size() > 25) most_popular_users0.pop_back();
  
  if (!most_popular_users0.empty()) {
    const table::Column & name_column = graph.getNodeArray().getTable()["name"];
    const table::Column & username_column = graph.getNodeArray().getTable()["uname"];
    for (auto & ud : most_popular_users0) {
      int i = graph.getNodeArray().getNodeId(ud.first.source_id, ud.first.source_object_id);
      string title;
      if (i == -1) {
	title = "(not available)";
      } else {
	string name, uname;
	name = name_column.getText(i);
	uname = username_column.getText(i);
	if (!uname.empty()) {
	  title = "@" + uname;
	} else {
	  title = name;
	}
      }
      output.most_popular_users.push_back({ ud.first.source_id, title, ud.second });
    }
  }

  for (auto & p : filter_usage) {
    output.top_filters.push_back(p);
  }
  
  for (auto & p : language_usage) {
    output.top_languages.push_back(p);
  }

  for (auto & p : political_parties) {
    output.top_political_parties.push_back(p);
  }

  std::map<skey, std::map<AppPlatform, int> > user_platforms;
  std::map<skey, std::map<AppInfo::Device, int> > user_devices;
  
  for (auto & ud : application_usage) {
    auto & key = ud.first;
    for (auto & ad : ud.second) {
      if (ad.first != -1) {
	auto app = AppRegistry::getInstance().getApp(ad.first);
	if (app.getId()) {
	  user_devices[key][app.getDevice()] += ad.second;
	  user_platforms[key][app.getPlatform()] += ad.second;
	} else {
	  cerr << "app " << ad.first << " not found\n";
	  user_devices[key][AppInfo::UNKNOWN_DEVICE] += ad.second;
	  user_platforms[key][UNKNOWN_PLATFORM] += ad.second;
	}
      } else {
	user_devices[key][AppInfo::UNKNOWN_DEVICE] += ad.second;
	user_platforms[key][UNKNOWN_PLATFORM] += ad.second;
      }
    }
  }
  
  for (auto & ud : user_devices) {
    AppInfo::Device best_dev = AppInfo::UNKNOWN_DEVICE;
    int best_count = 0;
    for (auto & p : ud.second) {
      if (p.first != AppInfo::UNKNOWN_DEVICE && p.second > best_count) {
	best_dev = p.first;
	best_count = p.second;
      }
    }
    output.device_users[best_dev]++;
  }

  for (auto & ud : user_platforms) {
    AppPlatform best_plat = UNKNOWN_PLATFORM;
    int best_count = 0;
    for (auto & p : ud.second) {
      if (p.second > best_count) {
	best_plat = p.first;
	best_count = p.second;
      }
    }
    if (best_count > 0) {
      output.platform_users[best_plat]++;
    }
  }

  output.hours = hours;
  output.weekdays = weekdays;
  output.user_types = user_types;
  output.political_parties = political_parties;
  
  output.sortData();
}

void
RawStatistics::addLink(const std::string & title, const std::string & url) {
  links[url]++;
  version++;
#if 0
  URI uri(url_lc);
  if (uri.getDomain() == "yle.fi" || // *
      uri.getDomain() == "www.mtv.fi" ||
      uri.getDomain() == "www.iltalehti.fi" ||
      uri.getDomain() == "www.iltasanomat.fi" || // *
      uri.getDomain() == "www.hs.fi" || // *
      uri.getDomain() == "www.ft.com" || // *
      uri.getDomain() == "www.taloussanomat.fi" // *
      ) {
    headlines[title]++;
  }
#endif
}

void
RawStatistics::addHashtag(const std::string & h) {
  hashtags[h]++;
  version++;
}

void
RawStatistics::addReceivedActivity(time_t t, short source_id, long long source_object_id, long long app_id, long long filter_id) {
  skey key(source_id, source_object_id);
  user_popularity[key]++;
  version++;
}

void
RawStatistics::addActivity(time_t t, short source_id, long long source_object_id, short lang, long long app_id, long long filter_id, PoliticalParty party) { 
  DateTime dt(t);
  assert(dt.getHour() >= 0 && dt.getHour() < 24);
  hours[dt.getHour()]++;
  assert(dt.getDayOfWeek() >= 1 && dt.getDayOfWeek() <= 7);
  weekdays[dt.getDayOfWeek() - 1]++;

  skey key(source_id, source_object_id);

  user_activity[key]++;

  if (lang) {
    language_usage[lang]++;
  }
  if (app_id != -1) {
    application_usage[key][app_id]++;
  }
  if (filter_id) {
    filter_usage[FilterType(filter_id)]++;
  }
  political_parties[party]++;

  version++;
}
