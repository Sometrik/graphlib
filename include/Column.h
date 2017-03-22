#ifndef _COLUMN_H_
#define _COLUMN_H_

#include <string>
#include <memory>
#include <vector>
#include <cassert>

namespace table {
  class ColumnBase {
  public:
  ColumnBase() { }

    ColumnBase & operator= (const ColumnBase & other) = delete;

    virtual ~ColumnBase() = default;

    virtual void reserve(size_t n) = 0;
    virtual size_t size() const = 0;

    virtual double getDouble(int i) const = 0;
    virtual int getInt(int i) const = 0;
    virtual long long getInt64(int i) const = 0;
    virtual std::string getText(int i) const = 0;
    
    virtual void setValue(int i, double v) = 0;
    virtual void setValue(int i, int v) = 0;
    virtual void setValue(int i, long long v) = 0;
    virtual void setValue(int i, const std::string & v) = 0;
    
    virtual bool compare(int a, int b) const { return false; }
    virtual void clear() = 0;

    virtual void pushValue(double v) = 0;
    virtual void pushValue(int v) = 0;
    virtual void pushValue(long long v) = 0;
    virtual void pushValue(const std::string & v) = 0;

    virtual void remove(int row) = 0;
  };

  template<class T>
  class Column : public ColumnBase {
  public:
    void reserve(size_t n) override { data.reserve(n); }
    size_t size() const override { return data.size(); }
    
    double getDouble(int i) const override { return (double)data[i]; }
    long long getInt64(int i) const override { return (long long)data[i]; }
    std::string getText(int i) const override { return std::to_string(data[i]); }
    int getInt(int i) const override { return (int)data[i]; }
    
    void setValue(int i, double v) override {
      assert(i >= 0);
      while (i >= data.size()) data.push_back(T());
      data[i] = T(v);
    }
    void setValue(int i, long long v) override {
      assert(i >= 0);
      while (i >= data.size()) data.push_back(T());
      data[i] = T(v);
    }
    void setValue(int i, const std::string & v) override {
      // setValue(i, stoi(v));
    }
    void setValue(int i, int v) override {
      assert(i >= 0);
      while (i >= data.size()) data.push_back(T());
      data[i] = T(v);
    }
    
    void pushValue(double v) override { data.push_back(T(v)); }
    void pushValue(const std::string & v) override {
      // data.push_back(v);
    }
    void pushValue(long long v) override {
      data.push_back(T(v));
    }
    void pushValue(int v) override {
      data.push_back(T(v));
    }

    bool compare(int a, int b) const override {
      return data[a] < data[b];
    }
    void clear() override { data.clear(); }

    void remove(int row) override {
      if (row >= 0 && row < data.size()) {
	data[row] = data.back();
	data.pop_back();
      }
    }

  private:
    std::vector<T> data;
  };

  class NullColumn : public ColumnBase {
  public:
  NullColumn() { }
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
    void remove(int row) override { }
  };
};

#endif
