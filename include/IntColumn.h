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
    
    double getDouble(int i) const override { return (double)getInt(i); }
    long long getInt64(int i) const override { return getInt(i); }
    std::string getText(int i) const override { return std::to_string(getInt(i)); }
    int getInt(int i) const override {
      if (i >= 0 && i < data.size()) {
	return data[i];
      } else {
	return 0;
      }
    }
    
    void setValue(int i, double v) override { setValue(i, (int)v); }
    void setValue(int i, long long v) override { setValue(i, (int)v); }
    void setValue(int i, const std::string & v) override { setValue(i, stoi(v)); }
    void setValue(int i, int v) override {
      while (i >= data.size()) data.push_back(0);
      data[i] = v;
    }
    
    void pushValue(double v) override { pushValue((int)v); }
    void pushValue(const std::string & v) override { pushValue(stoi(v)); }
    void pushValue(long long v) override { pushValue((int)v); }
    void pushValue(int v) override { data.push_back(v); }

    bool compare(int a, int b) const override {
      return data[a] < data[b];
    }
    void clear() override { data.clear(); }
    
  private:
    std::vector<int> data;
  };
};

#endif
