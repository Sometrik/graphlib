#ifndef _LOUVAINSIMPLIFIER_H_
#define _LOUVAINSIMPLIFIER_H_

#include "GraphFilter.h"

class LouvainSimplifier : public GraphFilter {
 public:
  LouvainSimplifier(int _max_levels);

  std::shared_ptr<GraphFilter> dup() const override { return std::make_shared<LouvainSimplifier>(max_levels); }

  bool apply(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats) override;
  
 private:
  int max_levels;
};

class LouvainSimplifierFactory : public GraphFilterFactory {
 public:
  LouvainSimplifierFactory(int _max_levels) : max_levels(_max_levels) { }

  virtual std::shared_ptr<GraphFilter> create() { return std::make_shared<LouvainSimplifier>(max_levels); }

 private:
  int max_levels;
};

#endif
