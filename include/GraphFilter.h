#ifndef _GRAPHFILTER_H_
#define _GRAPHFILTER_H_

#include <memory>
#include <ctime>

#include <unordered_set>
#include <unordered_map>

class Graph;
class RawStatistics;

class GraphFilter {
 public:
  GraphFilter() { }
  virtual ~GraphFilter() { }

  virtual std::shared_ptr<GraphFilter> dup() const = 0;
  virtual bool apply(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats) = 0;
  virtual void reset() {
    current_pos = -1;
    min_time = max_time = 0;
    num_links = num_hashtags = 0;
  }
  virtual bool hasPosition() const { return current_pos != -1; }

  void keepHashtags(bool t) { keep_hashtags = t; }
  void keepLinks(bool t) { keep_links = t; }
  void keepLang(bool t) { keep_lang = t; }
  void keepApplications(bool t) { keep_applications = t; }
  
 protected:
  bool processTemporalData(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats);

 private:
  int current_pos = -1;
  time_t min_time = 0, max_time = 0;
  unsigned int num_links = 0, num_hashtags = 0;
  bool keep_hashtags = false;
  bool keep_links = false;
  bool keep_lang = false;
  bool keep_applications = false;
};

class GraphFilterFactory {
 public:
  GraphFilterFactory() { }
  virtual ~GraphFilterFactory() { }

  virtual std::shared_ptr<GraphFilter> create() = 0;
};

#endif
