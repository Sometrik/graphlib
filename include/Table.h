#ifndef _TABLE_H_
#define _TABLE_H_

#include "Column.h"

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace table {    
  class Table {
  public:
  Table() : num_rows(0) { }
    
    bool hasColumn(const char * name) const {
      auto it = columns.find(name);
      return it != columns.end();
    }

    const Column * getColumnSafe(const char * name) const {
      auto it = columns.find(name);
      return it != columns.end() ? it->second.get() : 0;
    }
    
    Column & addTextColumn(const char * name);
    Column & addCompressedTextColumn(const char * name);  
    Column & addDoubleColumn(const char * name);
    Column & addIntColumn(const char * name);
    Column & addUShortColumn(const char * name);  
    Column & addBigIntColumn(const char * name);
    Column & addTimeSeriesColumn(const char * name);
    
    Column & addColumn(const std::shared_ptr<Column> & col) {
      col->reserve(num_rows);
      columns[col->name()] = col;
      columns_in_order.push_back(col);
      return *col;
    }

    void dropColumn(const char * name) {
      auto it = columns.find(name);
      if (it != columns.end()) columns.erase(it);
    }
    void dropColumn(const std::string & name) { dropColumn(name.c_str()); }

    void clear() {
      for (auto & col : columns) {
	col.second->clear();
      }
      num_rows = 0;
    }
    
    Column & operator[] (int i) {
      for (auto it = columns.begin(); it != columns.end(); it++, i--) {
	if (!i) return *(it->second);
      }
      return null_column;
    }
    
    const Column & operator[] (int i) const {
      for (auto it = columns.begin(); it != columns.end(); it++, i--) {
	if (!i) return *(it->second);
      }
      return null_column;
    }
    
    Column & operator[] (const char * s) {
      auto it = columns.find(s);
      if (it != columns.end()) {
	return *(it->second);
      } else {
	return null_column;
      }
    }
    
    const Column & operator[] (const char * s) const {
      auto it = columns.find(s);
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
    
    void addRow() { num_rows++; }

    void loadCSV(const char * filename, char delimiter = ';');
    
    size_t size() const { return num_rows; }
    bool empty() const { return num_rows == 0; }
    size_t getColumnCount() const { return columns.size(); }

    std::unordered_map<std::string, std::shared_ptr<Column> > & getColumns() { return columns; }
    const std::unordered_map<std::string, std::shared_ptr<Column> > & getColumns() const { return columns; }

  private:
    std::unordered_map<std::string, std::shared_ptr<Column> > columns;
    std::vector<std::shared_ptr<Column> > columns_in_order;
    NullColumn null_column;
    size_t num_rows;
  };
};

#endif
