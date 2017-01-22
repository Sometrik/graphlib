#ifndef _IMAGESET_H_
#define _IMAGESET_H_

#include <Column.h>

struct image_url_s {
  bool operator< (const image_url_s & other) const {
    if (width < other.width) return true;
    else if (width == other.width && height < other.height) return true;
    return false;
  }

  unsigned int width, height;
  std::string url;
};

class ImageSet {
 public:
  ImageSet() = default;

  const image_url_s & getSuitableUrl(unsigned int width) const {
    for (auto & i : images) {
      if (i.width >= width) {
	return i;
      }
    }
    if (images.empty()) {
      return null_image;
    } else {
      auto it = images.end();
      return *it;
    }
  }

  void insert(const image_url_s & image) { images.insert(image); }
  
 private:
  std::set<image_url_s> images;
  image_url_s null_image;
};

namespace table {
  class ImageSetColumn : public ColumnBase {
  public:
  ImageSetColumn(const std::string & _name) : ColumnBase(_name) { }
  ImageSetColumn(const char * _name) : ColumnBase(_name) { }
    
    // ColumnType getType() const override { return IMAGE_SRC; }

    size_t size() const override { return data.size(); }
    void reserve(size_t n) override { data.reserve(n); }
    
    double getDouble(int i) const override { return 0; }
    int getInt(int i) const override { return 0; }
    long long getInt64(int i) const override { return 0; }
    std::string getText(int i) const override { return 0; }

    void setValue(int i, double v) override { }
    void setValue(int i, int v) override { }
    void setValue(int i, long long v) override { }
    void setValue(int i, const std::string & v) override { }
    void addImage(int i, const image_url_s & image) { data[i].insert(image); }

    bool compare(int a, int b) const override { return false; }
    void clear() override { data.clear(); }

    void pushValue(double v) override {
      data.push_back(ImageSet());
    }
    void pushValue(int v) override {
      data.push_back(ImageSet());
    }
    void pushValue(long long v) override {
      data.push_back(ImageSet());
    }
    void pushValue(const std::string & v) override {
      data.push_back(ImageSet());
    }
            
  private:
    std::vector<ImageSet> data;
  };
};

#endif
