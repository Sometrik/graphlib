#ifndef _SIMILARITYSIMPLIFIER_H_
#define _SIMILARITYSIMPLIFIER_H_

#include "GraphFilter.h"

class SimilaritySimplifier : public GraphFilter {
 public:
  SimilaritySimplifier() { }

  std::shared_ptr<GraphFilter> dup() const override { return std::make_shared<SimilaritySimplifier>(); }

  bool apply(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats) override;
};

class SimilaritySimplifierFactory : public GraphFilterFactory {
 public:
  SimilaritySimplifierFactory() { }

  virtual std::shared_ptr<GraphFilter> create() { return std::make_shared<SimilaritySimplifier>(); }
};

#endif
