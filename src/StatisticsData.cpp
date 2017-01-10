#include "StatisticsData.h"

#include <algorithm>

using namespace std;

static bool compareQuantity(const statistics_row_s & a, const statistics_row_s & b) {
  return a.value > b.value;
}

static bool compareQuantity2(const pair<FilterType, int> & a, const pair<FilterType, int> & b) {
  return a.second > b.second;
}

static bool compareQuantity3(const pair<short, int> & a, const pair<short, int> & b) {
  return a.second > b.second;
}

static bool compareQuantity4(const pair<PoliticalParty, int> & a, const pair<PoliticalParty, int> & b) {
  return a.second > b.second;
}

void
StatisticsData::sortData() {
  sort(sorted_hashtags.begin(), sorted_hashtags.end(), compareQuantity);
  sort(sorted_links.begin(), sorted_links.end(), compareQuantity);
  sort(top_filters.begin(), top_filters.end(), compareQuantity2);
  sort(top_political_parties.begin(), top_political_parties.end(), compareQuantity4);
  sort(top_languages.begin(), top_languages.end(), compareQuantity3);
  while (sorted_hashtags.size() > 100) sorted_hashtags.pop_back();  
  while (sorted_links.size() > 100) sorted_links.pop_back();
}
