#ifndef _TABLE_DOUBLECOLUMN_H_
#define _TABLE_DOUBLECOLUMN_H_

#include <Column.h>

namespace table {
  class DoubleColumn : public Column {
  public:
  DoubleColumn(const std::string & _name) : Column(_name) { }
  DoubleColumn(const char * _name) : Column(_name) { }

    std::shared_ptr<Column> copy() const override { return std::make_shared<DoubleColumn>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<DoubleColumn>(name()); }
    void reserve(size_t n) { data.reserve(n); }     
    size_t size() const override { return data.size(); }
    
    ColumnType getType() const override { return FLOAT; }

    double getDouble(int i) const override { return data[i]; }
    int getInt(int i) const override { return (int)getDouble(i); }
    long long getInt64(int i) const override { return (long long)getDouble(i); }
    std::string getText(int i) const override {
      std::string s = std::to_string(data[i]);
      while (s[s.size() - 2] == '0' && s[s.size() - 1] == '0') s.erase(s.size() - 1);
      return s;
    }
    
    void setValue(int i, int v) override { setValue(i, (double)i); }
    void setValue(int i, long long v) override { setValue(i, (double)i); }
    void setValue(int i, const std::string & v) override { setValue(i, stof(v)); }
    void setValue(int i, double v) override {
      while (i >= data.size()) data.push_back(0);
      data[i] = v;
    }

    void pushValue(const std::string & v) override { pushValue(stof(v)); }
    void pushValue(long long v) override { pushValue((double)v); }
    void pushValue(int v) override { pushValue((double)v); }
    void pushValue(double v) override { data.push_back(v); }

    bool compare(int a, int b) const override {
      return data[a] < data[b];
    }
    void clear() override { data.clear(); }

    Column & operator= (double a) override { 
      data.assign(data.size(), a);
      return *this;
    }
    Column & operator= (int a) override {
      data.assign(data.size(), (double)a);
      return *this;
    }        
    
  private:
    std::vector<double> data;
  };
};

#endif
