#ifndef _DBASE3FILE_H_
#define _DBASE3FILE_H_

#include "Column.h"

#include <memory>
#include <vector>

namespace table {
  class DBase3Handle;
  
  class ColumnDBase3 : public Column {
  public:
    ColumnDBase3(std::shared_ptr<DBase3Handle> _dbf,
		 int _column_index,
		 int _num_rows,
		 const std::string & _name);

  std::shared_ptr<Column> copy() const override { return std::make_shared<ColumnDBase3>(*this); }
  std::shared_ptr<Column> create() const override { return std::shared_ptr<Column>(0); } // FIXME
  
    size_t size() const override { return num_rows; }
    
    double getDouble(int i) const override;
    int getInt(int i) const override;
    long long getInt64(int i) const override;
    std::string getText(int i) const override;
        
    void setValue(int i, double v) override { }
    void setValue(int i, int v) override { }
    void setValue(int i, long long v) override { }
    void setValue(int i, const std::string & v) override { }
    
    bool compare(int a, int b) const override { return getText(a) < getText(b); }
    void clear() override { }
    void addRow() override { }
    
    Column & operator= (double a) override {
      return *this;
    }
    Column & operator= (int a) override {
      return *this;
    }
    
  private:
    std::shared_ptr<DBase3Handle> dbf;
    int column_index;
    unsigned int num_rows;
  };

  class DBase3File {
  public:
    DBase3File(const std::string & filename);
    
    unsigned int getRecordCount() const { return record_count; }
    std::vector<std::shared_ptr<Column> > & getColumns() { return columns; }
    
  private:
    bool openDBF(const std::string & filename);
    
    std::shared_ptr<DBase3Handle> dbf;
    unsigned int record_count;
    std::vector<std::shared_ptr<Column> > columns;
  };
};

#endif

