#ifndef _TABLE_TEXTCOLUMN_H_
#define _TABLE_TEXTCOLUMN_H_

#include "Column.h"

#include <cstring>

namespace table {
  class TextColumn : public ColumnBase {
  public:
  TextColumn() { }
  TextColumn(const TextColumn & other) : ColumnBase(other) {
      auto & other_data = other.data;
      for (int i = 0; i < other_data.size(); i++) {
	pushValue(other_data[i]);
      }      
    }
    ~TextColumn() {
      for (int i = 0; i < data.size(); i++) {
	delete[] data[i];
      }
    }
    
    size_t size() const override { return data.size(); }
    void reserve(size_t n) override { data.reserve(n); }
    
    double getDouble(int i) const override { return 0; }
    int getInt(int i) const override { return 0; }
    long long getInt64(int i) const override { return data[i] ? stoll(std::string(data[i])) : 0; }
    std::string getText(int i) const override {
      if (i >= 0 && i < data.size()) {
	return data[i] ? data[i] : "";
      } else {
	return "";
      }
    }
    
    void setValue(int i, double v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, int v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, long long v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, const std::string & v) override { setValue(i, v.c_str(), v.size()); }
    void setValue(int i, const char * v, const size_t len) {
      while (i >= data.size()) data.push_back(0);
      delete[] data[i];
      if (v) {
	data[i] = new char[len + 1];
	memcpy(data[i], v, len + 1);
      } else {
	data[i] = 0;
      }
    }

    void pushValue(double v) override { pushValue(std::to_string(v)); }
    void pushValue(int v) override { pushValue(std::to_string(v)); }
    void pushValue(long long v) override { pushValue(std::to_string(v)); }
    void pushValue(const std::string & v) override { pushValue(v.c_str(), v.size()); }
    void pushValue(const char * v, const size_t len) {
      if (v) {
	data.push_back(new char[len + 1]);
	memcpy(data.back(), v, len + 1);
      } else {
	data.push_back(0);
      }
    }
        
    bool compare(int a, int b) const override { return strcmp(data[a] ? data[a] : "", data[b] ? data[b] : "") < 0; }
    void clear() override {
      for (int i = 0; i < data.size(); i++) {
	delete[] data[i];
      }
      data.clear();
    }

    void remove(int row) override {
      if (row >= 0 && row < data.size()) {
	delete[] data[row];
	data[row] = data.back();
	data.pop_back();
      }
    }

  private:
    std::vector<char *> data;
  };
};

#endif
