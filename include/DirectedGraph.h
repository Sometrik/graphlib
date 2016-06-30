#ifndef _DIRECTEDGRAPH_H_
#define _DIRECTEDGRAPH_H_

#include <Graph.h>
#include <GraphFilter.h>

class DirectedGraph : public Graph {
 public:
  DirectedGraph(int _id = 0);
  DirectedGraph(const DirectedGraph & other);

  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new DirectedGraph(*this); }
  bool isDirected() const override { return true; }
  bool hasPosition() const override { return filter.get() ? filter->hasPosition() : false; }

  bool updateData(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) override;
  
  void reset() override {
    if (filter.get()) filter->reset();    
  }

 private:
  std::shared_ptr<GraphFilter> filter;
};

#endif
