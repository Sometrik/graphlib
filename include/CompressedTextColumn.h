#ifndef _TABLE_COMPRESSEDTEXTCOLUMN_H_
#define _TABLE_COMPRESSEDTEXTCOLUMN_H_

#include "Column.h"

#include <Deflate.h>

#include <iostream>
#include <cstring>

#define MAX_BLOCK_SIZE 1048576

namespace table {
  class CompressedTextColumn : public Column {
  public:
  CompressedTextColumn(const std::string & _name) : Column(_name) { }
  CompressedTextColumn(const CompressedTextColumn & other) : Column(other.name()) {
      auto & other_data = other.data;
      for (int i = 0; i < other_data.size(); i++) {
	addRow();
	setValue(i, other_data[i], other_data[i] ? strlen(other_data[i]) : 0);
      }      
    }
    ~CompressedTextColumn() {
      for (int i = 0; i < data.size(); i++) {
	delete[] data[i];
      }
    }
    
    std::shared_ptr<Column> copy() const override { return std::make_shared<CompressedTextColumn>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<CompressedTextColumn>(name()); }
    size_t size() const override { return data.size(); }
    
    double getDouble(int i) const override { return 0; }
    int getInt(int i) const override { return 0; }
    long long getInt64(int i) const override { return 0; }
    std::string getText(int i) const override {
      if (i >= 0 && i < data.size()) {
	return data[i] ? data[i] : "";
      } else {
	std::cerr << "invalid CompressedTextColumn access (i = " << i << ", size = " << data.size() << ")" << std::endl;
	return 0;
      }
    }
    
    void setValue(int i, double v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, int v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, long long v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, const std::string & v) override { setValue(i, v.c_str(), v.size()); }
    void setValue(int i, const char * v, const size_t len) {
      if (len) {
	unsigned int block_num = 1 + compressed_blocks.size();
	unsigned int index = active_block.compress(v, len);
	data[i] = std::pair<unsigned int, unsigned int>(block_num, index);
	if (active_block.size() >= MAX_BLOCK_SIZE) {
	  compressed_blocks.push_back(active_block.data());
	  active_block.reset();
	}
      } else if (data[i].first) {
	data[i].first = data[i].second = 0;
      }      
    }
    
    bool compare(int a, int b) const override { return 0; }
    void clear() override {
      data.clear();
      compressed_blocks.clear();
      active_block.reset();      
    }
        
  private:
    std::vector<std::pair<unsigned int, unsigned int> > data;
    std::vector<std::basic_string<unsigned char> > compressed_blocks;
    Deflate active_block;
  };
};

#endif
