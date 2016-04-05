#ifndef _TABLE_BIGINTCOLUMN_H_
#define _TABLE_BIGINTCOLUMN_H_

#include <Column.h>

namespace table {
  class BigIntColumn : public Column {
  public:
  BigIntColumn(const std::string & _name) : Column(_name) { }
  BigIntColumn(const char * _name) : Column(_name) { }
    
    ColumnType getType() const override { return NUMBER; }

    std::shared_ptr<Column> copy() const override { return std::make_shared<BigIntColumn>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<BigIntColumn>(name()); }
    void reserve(size_t n) { data.reserve(n); }
    size_t size() const override { return data.size(); }
    
    double getDouble(int i) const override { return (double)getInt64(i); }
    int getInt(int i) const override { return (int)getInt64(i); }
    std::string getText(int i) const override { return std::to_string(getInt64(i)); }
    long long getInt64(int i) const override {
      if (i >= 0 && i < data.size()) {
	return data[i];
      } else {
	return 0;
      }
    }
    
    void setValue(int i, double v) override { setValue(i, (long long)v); }
    void setValue(int i, int v) override { setValue(i, (int)v); }
    void setValue(int i, const std::string & v) override { setValue(i, stoll(v)); }
    void setValue(int i, long long v) override {
      while (i >= data.size()) data.push_back(0);
      data[i] = v;
    }
	
    void pushValue(double v) override { pushValue((long long)v); }
    void pushValue(int v) override { pushValue((long long)v); }
    void pushValue(const std::string & v) override { pushValue(stoll(v)); }
    void pushValue(long long v) override { data.push_back(v); }
    
    bool compare(int a, int b) const override {
      return data[a] < data[b];
    }
    void clear() override { data.clear(); }
    
  private:
    std::vector<long long> data;
  };
};

#endif
