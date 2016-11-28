#ifndef _GRAPHFILTER_H_
#define _GRAPHFILTER_H_

#include <memory>
#include <ctime>

class Graph;
class RawStatistics;

class GraphFilter {
 public:
  GraphFilter() { }
  virtual ~GraphFilter() { }

  virtual std::shared_ptr<GraphFilter> dup() const = 0;
  virtual bool updateData(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) = 0;
  virtual void reset() { }
  virtual bool hasPosition() const { return false; }
};

class GraphFilterFactory {
 public:
  GraphFilterFactory() { }
  virtual ~GraphFilterFactory() { }

  virtual std::shared_ptr<GraphFilter> create() = 0;
};

#endif