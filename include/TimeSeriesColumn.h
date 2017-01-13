#ifndef _TIMESERIESCOLUMN_H_
#define _TIMESERIESCOLUMN_H_

#include "Column.h"

#include <vector>
#include <string>
#include <map>

namespace table {
  class TimeSeriesColumn : public ColumnBase {
  public:
    TimeSeriesColumn(const std::string & _name) : ColumnBase(_name) { }
    TimeSeriesColumn(const char * _name) : ColumnBase(_name) { }

    std::shared_ptr<ColumnBase> copy() const override { return std::make_shared<TimeSeriesColumn>(*this); }
    std::shared_ptr<ColumnBase> create() const override { return std::make_shared<TimeSeriesColumn>(name()); }
    
    // ColumnType getType() const override { return TIME_SERIES; }

    size_t size() const override { return data.size(); }
    void reserve(size_t n) override { data.reserve(n); }

    double getDouble(int i) const override { return (double)data[i].size(); }
    int getInt(int i) const override { return (int)data[i].size(); }
    long long getInt64(int i) const override { return (long long)data[i].size(); }
    std::string getText(int i) const override { return std::to_string(data[i].size()); }

    void setValue(int i, double v) override { }
    void setValue(int i, int v) override { }
    void setValue(int i, long long v) override { }
    void setValue(int i, const std::string & v) override { }
    void addValue(int i, time_t t, double val) {
      while (i >= data.size()) data.push_back(std::map<time_t, double>());
      data[i][t] = val;
    }

    void setValues(int i, const std::map<time_t, double> & values) { data[i] = values; }
    const std::map<time_t, double> & getValues(int i) const { return data[i]; }

    bool compare(int a, int b) const override { return false; }
    void clear() override { data.clear(); }

    void pushValue(double v) override {
      data.push_back(std::map<time_t, double>());
    }
    void pushValue(int v) override {
      data.push_back(std::map<time_t, double>());
    }
    void pushValue(long long v) override {
      data.push_back(std::map<time_t, double>());
    }
    void pushValue(const std::string & v) override {
      data.push_back(std::map<time_t, double>());
    }
    
  private:
    std::vector<std::map<time_t, double> > data;
  };
};

#endif
