#ifndef _COLUMN_H_
#define _COLUMN_H_

#include <string>
#include <memory>

namespace table {
  enum ColumnType {
    TEXT = 1,
    NUMBER,
    FLOAT,
    BOOL,
    TIME_SERIES,
    IMAGE_SRC
  };

  class Column {
  public:
  Column(const std::string & _name) : column_name(_name) { }
  Column(const char * _name) : column_name(_name) { }

    Column(const Column & other) = delete;
    Column & operator= (const Column & other) = delete;

    virtual ~Column() { }

    virtual std::shared_ptr<Column> copy() const = 0;
    virtual std::shared_ptr<Column> create() const = 0;    
    virtual void reserve(size_t n) = 0;
    virtual size_t size() const = 0;

    virtual ColumnType getType() const { return TEXT; }    
    virtual double getDouble(int i) const = 0;
    virtual int getInt(int i) const = 0;
    virtual long long getInt64(int i) const = 0;
    virtual std::string getText(int i) const = 0;
    
    virtual void setValue(int i, double v) = 0;
    virtual void setValue(int i, int v) = 0;
    virtual void setValue(int i, long long v) = 0;
    virtual void setValue(int i, const std::string & v) = 0;
    // virtual Column magnitude() const { return *this; }
    
    virtual bool compare(int a, int b) const { return false; }
    virtual void clear() = 0;

    virtual void pushValue(double v) = 0;
    virtual void pushValue(int v) = 0;
    virtual void pushValue(long long v) = 0;
    virtual void pushValue(const std::string & v) = 0;
    
    const char * getTypeText() const {
      switch (getType()) {
      case TEXT: return "string";
      case NUMBER: return "int";
      case FLOAT: return "double";
      case BOOL: return "bool";
      case TIME_SERIES: return "time_series";
      case IMAGE_SRC: return "image_src";
      }
      return "";
    }
    
    const std::string & name() const { return column_name; }
    
  private:
    std::string column_name;
  };
  
  class NullColumn : public Column {
  public:
  NullColumn() : Column("") { }
    std::shared_ptr<Column> copy() const override { return std::make_shared<NullColumn>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<NullColumn>(); }
    void reserve(size_t n) override { }
    size_t size() const override { return 0; }
    double getDouble(int i) const override { return 0; }
    int getInt(int i) const override { return 0; }
    long long getInt64(int i) const override { return 0; }
    std::string getText(int i) const override { return ""; }
    void setValue(int i, double v) override { }
    void setValue(int i, int v) override { }
    void setValue(int i, long long v) override { }
    void setValue(int i, const std::string & v) override { }
    void clear() override { }

    void pushValue(double v) override { }
    void pushValue(int v) override { }
    void pushValue(long long v) override { }
    void pushValue(const std::string & v) override { }
  };
};

#endif
