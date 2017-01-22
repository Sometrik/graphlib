#ifndef _TABLE_COMPRESSEDTEXTCOLUMN_H_
#define _TABLE_COMPRESSEDTEXTCOLUMN_H_

#include "Column.h"

#include <Deflate.h>
#include <Inflate.h>

#include <iostream>
#include <string>
#include <cstring>

#define MAX_BLOCK_SIZE 1048576

namespace table {
  struct data_ptr_s {
    unsigned short block_number, data_length;
    unsigned int data_offset;
  };
  
  class CompressedTextColumn : public ColumnBase {
  public:
  CompressedTextColumn(const std::string & _name) : ColumnBase(_name) { }
    
    size_t size() const override { return data.size(); }
    void reserve(size_t n) override { data.reserve(n); }
    
    double getDouble(int i) const override { return 0; }
    int getInt(int i) const override { return 0; }
    long long getInt64(int i) const override { return 0; }
    std::string getText(int i) const override {
      if (i >= 0 && i < data.size()) {
	auto & p = data[i];
	if (p.data_length) {
	  if (p.block_number < compressed_blocks.size()) {
	    Inflate inflate(&(compressed_blocks[p.block_number]));
	    return inflate.decompressString(p.data_offset, p.data_length);
	  } else {
	    Deflate tmp(active_block);
	    tmp.flush();
	    Inflate inflate(&(tmp.data()));
	    return inflate.decompressString(p.data_offset, p.data_length);
	  }
	}
      }
      return "";
    }
    
    void setValue(int i, double v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, int v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, long long v) override { setValue(i, std::to_string(v)); }
    void setValue(int i, const std::string & v) override { setValue(i, v.c_str(), v.size()); }
    void setValue(int i, const char * v, const size_t len) {
      // std::cerr << "setValue(" << i << ", " << v << ", " << len << ")\n";
      while (i >= data.size()) data.push_back({ 0, 0, 0 });
      if (len) {
	data[i] = compressValue(v, len);
      } else if (data[i].data_length) {
	data[i] = { 0, 0, 0 };
      }      
    }

    void pushValue(double v) override { pushValue(std::to_string(v)); }
    void pushValue(int v) override { pushValue(std::to_string(v)); }
    void pushValue(long long v) override { pushValue(std::to_string(v)); }
    void pushValue(const std::string & v) override { pushValue(v.c_str(), v.size()); }
    void pushValue(const char * v, const size_t len) {
      if (len) {
	data.push_back(compressValue(v, len));      
      } else {
	data.push_back({ 0, 0, 0 });
      }
    }    

    bool compare(int a, int b) const override { return 0; }
    void clear() override {
      data.clear();
      compressed_blocks.clear();
      active_block.reset();
      uncompressed_size = compressed_size = 0;
    }
        
  private:
    data_ptr_s compressValue(const char * v, size_t len) {
      unsigned short block_num = compressed_blocks.size();
      unsigned int offset = active_block.compress(v, len);
      if (active_block.size() >= MAX_BLOCK_SIZE) {
	active_block.flush();
	compressed_blocks.push_back(active_block.data());
	compressed_size += active_block.size();
	active_block.reset();
      }
      uncompressed_size += len;
      unsigned int tmp = compressed_size + active_block.size();
      std::cerr << "compressed text " << v << " (blocks = " << (compressed_blocks.size() + 1) << ", total = " << uncompressed_size << ", compressed = " << tmp << ", ratio = " << (100.0 * tmp / uncompressed_size) << std::endl;

      return { block_num, (unsigned short)len, offset };
    }

    std::vector<data_ptr_s> data;
    std::vector<std::basic_string<unsigned char> > compressed_blocks;
    Deflate active_block;
    unsigned int uncompressed_size = 0, compressed_size = 0;
  };
};

#endif
