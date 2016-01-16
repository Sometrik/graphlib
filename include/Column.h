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
    Column & operator= (const Column & other) = delete;

    virtual ~Column() { }

    virtual std::shared_ptr<Column> copy() const = 0;
    virtual std::shared_ptr<Column> create() const = 0;    

    void reserve(size_t n) {
      while (size() < n) addRow();
    }

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
    
    virtual void addRow() = 0;
    virtual bool compare(int a, int b) const { return false; }
    virtual void clear() = 0;

    virtual Column & operator= (double a) = 0;
    virtual Column & operator= (int a) = 0;

    const char * getTypeText() const {
      switch (getType()) {
      case NUMBER: return "int";
      case FLOAT: return "double";
      case TEXT:
      default:
	return "string";
      }
    }
    
    const std::string & name() const { return column_name; }
    bool isVisible() const { return is_visible; }
    void setVisible(bool t) { is_visible = t; }
    
  private:
    std::string column_name;
    bool is_visible = true;
  };
  
  class NullColumn : public Column {
  public:
  NullColumn() : Column("") { }
    std::shared_ptr<Column> copy() const override { return std::make_shared<NullColumn>(*this); }
    std::shared_ptr<Column> create() const override { return std::make_shared<NullColumn>(); }
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
    void addRow() override { }
    Column & operator= (double a) override { return *this; }
    Column & operator= (int a) override { return *this; }
  };
};

#endif
