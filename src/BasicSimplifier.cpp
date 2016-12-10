#include "BasicSimplifier.h"

#include <Graph.h>
#include <iostream>

using namespace std;

bool
BasicSimplifier::apply(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats) {
  bool is_changed = processTemporalData(target_graph, start_time, end_time, start_sentiment, end_sentiment, source_graph, stats);

  return is_changed;
}
