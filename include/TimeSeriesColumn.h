#ifndef _TIMESERIESCOLUMN_H_
#define _TIMESERIESCOLUMN_H_

#include "Column.h"

#include <vector>
#include <string>
#include <map>

namespace table {
  class TimeSeriesColumn : public Column {
  public:
    TimeSeriesColumn(const std::string & _name) : Column(_name) { }
    TimeSeriesColumn(const char * _name) : Column(_name) { }

    std::shared_ptr<Column> copy() const override { return std::make_shared<TimeSeriesColumn>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<TimeSeriesColumn>(name()); }
    
    ColumnType getType() const override { return TIME_SERIES; }

    size_t size() const override { return data.size(); }
    
    double getDouble(int i) const override { return (double)data[i].size(); }
    int getInt(int i) const override { return (int)data[i].size(); }
    long long getInt64(int i) const override { return (long long)data[i].size(); }
    std::string getText(int i) const override { return std::to_string(data[i].size()); }

    void setValue(int i, double v) override { }
    void setValue(int i, int v) override { }
    void setValue(int i, long long v) override { }
    void setValue(int i, const std::string & v) override { }
    void addValue(int i, time_t t, double val) { data[i][t] = val; }

    void setValues(int i, const std::map<time_t, double> & values) { data[i] = values; }
    const std::map<time_t, double> & getValues(int i) const { return data[i]; }

    bool compare(int a, int b) const override { return false; }
    void clear() override { data.clear(); }

    Column & operator= (double a) override { return *this; }
    Column & operator= (int a) override { return *this; }
        
    void addRow() override {
      data.push_back(std::map<time_t, double>());
    }
    
  private:
    std::vector<std::map<time_t, double> > data;
  };
};

#endif
