#ifndef _BASICSIMPLIFIER_H_
#define _BASICSIMPLIFIER_H_

#include "GraphFilter.h"

class BasicSimplifier : public GraphFilter {
 public:
  BasicSimplifier() { }

  std::shared_ptr<GraphFilter> dup() const override { return std::make_shared<BasicSimplifier>(); }

  bool apply(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats) override;
};

class BasicSimplifierFactory : public GraphFilterFactory {
 public:
  BasicSimplifierFactory() { }

  virtual std::shared_ptr<GraphFilter> create() { return std::make_shared<BasicSimplifier>(); }
};

#endif
