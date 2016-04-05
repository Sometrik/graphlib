#ifndef _TABLE_INTCOLUMN_H_
#define _TABLE_INTCOLUMN_H_

#include "Column.h"

namespace table {
  class IntColumn : public Column {
  public:
  IntColumn(const std::string & _name) : Column(_name) { }
  IntColumn(const char * _name) : Column(_name) { }
    
    ColumnType getType() const override { return NUMBER; }

    std::shared_ptr<Column> copy() const override { return std::make_shared<IntColumn>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<IntColumn>(name()); }
    void reserve(size_t n) { data.reserve(n); }
    size_t size() const override { return data.size(); }
    
    double getDouble(int i) const override { return (double)data[i]; }
    int getInt(int i) const override { return data[i]; }
    long long getInt64(int i) const override { return data[i]; }
    std::string getText(int i) const override { return std::to_string(data[i]); }
    
    void setValue(int i, double v) override { }
    void setValue(int i, int v) override { data[i] = v; }
    void setValue(int i, long long v) override { data[i] = v; }
    void setValue(int i, const std::string & v) override { data[i] = stoi(v); }

    void pushValue(double v) override { pushValue((int)v); }
    void pushValue(const std::string & v) override { pushValue(stoi(v)); }
    void pushValue(long long v) override { pushValue((int)v); }
    void pushValue(int v) override { data.push_back(v); }

    bool compare(int a, int b) const override {
      return data[a] < data[b];
    }
    void clear() override { data.clear(); }

    Column & operator= (double a) override { 
      data.assign(data.size(), int(a));
      return *this;
    }
    Column & operator= (int a) override {
      data.assign(data.size(), a);
      return *this;
    }
    
  private:
    std::vector<int> data;
  };
};

#endif
