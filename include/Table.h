#ifndef _TABLE_H_
#define _TABLE_H_

#include "Column.h"
#include "TimeSeriesColumn.h"

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <cstring>
#include <iostream>

namespace table {

#if 0
  class Value {
  public:
    Value();
    
  private:
  };
  class Shape {
    size_t rows, columns;
  }
#endif
  
  class ColumnDouble : public Column {
  public:
  ColumnDouble(const std::string & _name) : Column(_name) { }
  ColumnDouble(const char * _name) : Column(_name) { }

    std::shared_ptr<Column> copy() const override { return std::make_shared<ColumnDouble>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<ColumnDouble>(name()); }

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
    
    void setValue(int i, double v) override { data[i] = v; }
    void setValue(int i, int v) override { data[i] = v; }
    void setValue(int i, long long v) override { data[i] = v; }
    void setValue(int i, const std::string & v) override { data[i] = stof(v); }
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
    void addRow() override {
      data.push_back(0.0);
    }
    
  private:
    std::vector<double> data;
  };

  class ColumnInt : public Column {
  public:
  ColumnInt(const std::string & _name) : Column(_name) { }
  ColumnInt(const char * _name) : Column(_name) { }
    
    ColumnType getType() const override { return NUMBER; }

    std::shared_ptr<Column> copy() const override { return std::make_shared<ColumnInt>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<ColumnInt>(name()); }
    size_t size() const override { return data.size(); }
    
    double getDouble(int i) const override { return (double)data[i]; }
    int getInt(int i) const override { return data[i]; }
    long long getInt64(int i) const override { return data[i]; }
    std::string getText(int i) const override { return std::to_string(data[i]); }
    
    void setValue(int i, double v) override { }
    void setValue(int i, int v) override { data[i] = v; }
    void setValue(int i, long long v) override { data[i] = v; }
    void setValue(int i, const std::string & v) override { data[i] = stoi(v); }
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
    void addRow() override {
      data.push_back(0);
    }
    
  private:
    std::vector<int> data;
  };

  class ColumnBigInt : public Column {
  public:
  ColumnBigInt(const std::string & _name) : Column(_name) { }
  ColumnBigInt(const char * _name) : Column(_name) { }
    
    ColumnType getType() const override { return NUMBER; }

    std::shared_ptr<Column> copy() const override { return std::make_shared<ColumnBigInt>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<ColumnBigInt>(name()); }
    size_t size() const override { return data.size(); }
    
    double getDouble(int i) const override { return (double)data[i]; }
    int getInt(int i) const override { return (int)data[i]; }
    long long getInt64(int i) const override { return data[i]; }
    std::string getText(int i) const override { return std::to_string(data[i]); }
    
    void setValue(int i, double v) override { }
    void setValue(int i, int v) override { data[i] = v; }
    void setValue(int i, long long v) override { data[i] = v; }
    void setValue(int i, const std::string & v) override { data[i] = stoll(v); }
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
    void addRow() override {
      data.push_back(0);
    }
    
  private:
    std::vector<long long> data;
  };

  class ColumnText : public Column {
  public:
  ColumnText(const std::string & _name) : Column(_name) { }
  ColumnText(const ColumnText & other) : Column(other.name()) {
      auto & other_data = other.data;
      for (int i = 0; i < other_data.size(); i++) {
	addRow();
	setValue(i, other_data[i], other_data[i] ? strlen(other_data[i]) : 0);
      }      
    }
    ~ColumnText() {
      for (int i = 0; i < data.size(); i++) {
	delete[] data[i];
      }
    }
    
    std::shared_ptr<Column> copy() const override { return std::make_shared<ColumnText>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<ColumnText>(name()); }
    size_t size() const override { return data.size(); }
    
    double getDouble(int i) const override { return 0; }
    int getInt(int i) const override { return 0; }
    long long getInt64(int i) const override { return 0; }
    std::string getText(int i) const override {
      if (i >= 0 && i < data.size()) {
	return data[i] ? data[i] : "";
      } else {
	std::cerr << "invalid ColumnText access (i = " << i << ", size = " << data.size() << ")" << std::endl;
	return 0;
      }
    }
    
    void setValue(int i, double v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, int v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, long long v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, const std::string & v) override { setValue(i, v.c_str(), v.size()); }
    void setValue(int i, const char * v, const size_t len) {
      delete[] data[i];
      if (v) {
	data[i] = new char[len + 1];
	memcpy(data[i], v, len + 1);
      } else {
	data[i] = 0;
      }
    }
    
    bool compare(int a, int b) const override { return strcmp(data[a] ? data[a] : "", data[b] ? data[b] : "") < 0; }
    void clear() override {
      for (int i = 0; i < data.size(); i++) {
	delete[] data[i];
      }
      data.clear();
    }
    
    void addRow() override { data.push_back(0);  }

    Column & operator= (double a) override {
      *this = std::to_string(a);
      return *this;
    }
    Column & operator= (int a) override {
      *this = std::to_string(a);
      return *this;
    }
    Column & operator= (const std::string & a) {
      for (int i = 0; i < data.size(); i++) {
	setValue(i, a);
      }
      return *this;
    }
    
  private:
    std::vector<char *> data;
  };
  
  class Table {
  public:
  Table() : num_rows(0) { }
    
    bool hasColumn(const char * name) const {
      std::map<std::string, std::shared_ptr<Column> >::const_iterator it = columns.find(name);
      return it != columns.end();
    }

    const Column * getColumnSafe(const char * name) const {
      std::map<std::string, std::shared_ptr<Column> >::const_iterator it = columns.find(name);
      return it != columns.end() ? it->second.get() : 0;
    }
    
    Column & addTextColumn(const char * name) {
      std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.find(name);
      if (it != columns.end()) {
	return *(it->second);
      } else {
	return addColumn(std::make_shared<ColumnText>(name));
      }
    }
    
    Column & addDoubleColumn(const char * name) {
      std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.find(name);
      if (it != columns.end()) {
	return *(it->second);
      } else {
	return addColumn(std::make_shared<ColumnDouble>(name));
      }
    }

    Column & addIntColumn(const char * name) {
      std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.find(name);
      if (it != columns.end()) {
	return *(it->second);
      } else {
	return addColumn(std::make_shared<ColumnInt>(name));
      }
    }

    Column & addBigIntColumn(const char * name) {
      std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.find(name);
      if (it != columns.end()) {
	return *(it->second);
      } else {
	return addColumn(std::make_shared<ColumnBigInt>(name));
      }
    }

    Column & addTimeSeriesColumn(const char * name) {
      std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.find(name);
      if (it != columns.end()) {
	return *(it->second);
      } else {
	return addColumn(std::make_shared<TimeSeriesColumn>(name));
      }
    }
    
    Column & addColumn(const std::shared_ptr<Column> & col) {
      if (col->size() < num_rows) {
	col->reserve(num_rows);
      } else if (col->size() > num_rows) {
	num_rows = col->size();
	for (std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.begin(); it != columns.end(); it++) {
	  it->second->reserve(num_rows);
	}
      }
      columns[col->name()] = col;
      columns_in_order.push_back(col);
      return *col;
    }

    void dropColumn(const char * name) {
      std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.find(name);
      if (it != columns.end()) columns.erase(it);
    }
    void dropColumn(const std::string & name) { dropColumn(name.c_str()); }

    void clear() {
      for (std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.begin(); it != columns.end(); it++) {
	it->second->clear();
      }
      num_rows = 0;
    }
    
    Column & operator[] (int i) {
      for (std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.begin(); it != columns.end(); it++, i--) {
	if (!i) return *(it->second);
      }
      return null_column;
    }
    
    const Column & operator[] (int i) const {
      for (std::map<std::string, std::shared_ptr<Column> >::const_iterator it = columns.begin(); it != columns.end(); it++, i--) {
	if (!i) return *(it->second);
      }
      return null_column;
    }
    
    Column & operator[] (const char * s) {
      std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.find(s);
      if (it != columns.end()) {
	return *(it->second);
      } else {
	return null_column;
      }
    }
    
    const Column & operator[] (const char * s) const {
      std::map<std::string, std::shared_ptr<Column> >::const_iterator it = columns.find(s);
      if (it != columns.end()) {
	return *(it->second);
      } else {
	return null_column;
      }
    }
    
    Column & operator[] (const std::string & s) {
      return (*this)[s.c_str()];
    }
    
    const Column & operator[] (const std::string & s) const {
      return (*this)[s.c_str()];
    }
    
    void addRow() {
      num_rows++;
      for (std::map<std::string, std::shared_ptr<Column> >::iterator it = columns.begin(); it != columns.end(); it++) {
	while (it->second->size() < size()) {
	  it->second->addRow();
	}
      }      
    }

    void loadCSV(const char * filename, char delimiter = ';');
    
    size_t size() const { return num_rows; }
    size_t getColumnCount() const { return columns.size(); }

    std::map<std::string, std::shared_ptr<Column> > & getColumns() { return columns; }
    const std::map<std::string, std::shared_ptr<Column> > & getColumns() const { return columns; }

  private:
    std::map<std::string, std::shared_ptr<Column> > columns;
    std::vector<std::shared_ptr<Column> > columns_in_order;
    NullColumn null_column;
    size_t num_rows;
  };
};

#endif
